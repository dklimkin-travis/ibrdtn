/*
Copyright 2017 Google LLC.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "dtnprotoping.h"

#include "protos/dtnping.pb.h"
#include "protos/dtnservice.grpc.pb.h"

#include <future>
#include <sstream>

using dtn::api::DtnSendRequest;
using dtn::api::DtnService;
using dtn::api::DtnSubscribeRequest;
using dtn::api::DtnSendResponse;
using dtn::tools::DtnPing;

DEFINE_string(src, "", "Set the source application name");
DEFINE_string(host, "localhost:4592", "gRPC API host and port");
DEFINE_bool(nowait, false, "Do not wait for a reply");
DEFINE_bool(abortfail, false, "Do not wait for a reply");
DEFINE_int32(size, 64, "The size of the payload");
DEFINE_int32(count, 10, "Send this many echos in a row");
DEFINE_int32(delay, 1, "Delay between echo requests");
DEFINE_int32(lifetime, 30, "Set the lifetime of outgoing bundles");
DEFINE_bool(encrypt, false, "Request encryption on the bundle layer");
DEFINE_bool(sign, false, "Request signature on the bundle layer");

const char* DtnProtoPing::GetUsageString()
{
	static constexpr char usage[] =
		"send echo request over DTN network (IBR-DTN / gRPC API)\n"
		"Syntax: dtnprotoping [flags] <dst>\n"
		"Where:\n  <dst> the destination node URL (e.g. dtn://node/echo)";
  	return usage;
}

void DtnProtoPing::Init(int* argc, char*** argv)
{
	DtnProtoTool::Init(argc, argv);
	if (*argc != 2)
	{
    	PrintHelp();
		exit(1);
  	}
	destinationUrl = (*argv)[1];
}

int DtnProtoPing::Execute()
{
	// To subscribe to a group we need a unique app ID, similar to TCP port.
	std::string appName = FLAGS_src;
	if (appName.empty())
	{
		std::srand(std::time(0));
		appName = std::string(gflags::ProgramInvocationShortName()) + ":" + std::to_string(std::rand());
	}

	auto args = grpc::ChannelArguments();
	args.SetMaxReceiveMessageSize(MAX_PROTO_MESSAGE_SIZE);
	args.SetCompressionAlgorithm(GRPC_COMPRESS_NONE);
	auto channel = grpc::CreateCustomChannel(FLAGS_host, grpc::InsecureChannelCredentials(), args);
  	std::unique_ptr<DtnService::Stub> dtnService(DtnService::NewStub(channel));

	// Receiver.
	auto fut(std::async(std::launch::async, &DtnProtoPing::ReadResponses, this, dtnService.get(), appName));

	// Sender.
	auto delayDuration = std::chrono::seconds(FLAGS_delay);
	DtnSendRequest req;

	req.set_ttl(FLAGS_lifetime);
  	req.set_client_id(appName);
  	req.set_destination_url(destinationUrl);
  	req.set_priority(dtn::api::Priority::NORMAL);

	int payloadSize = FLAGS_size;
	std::string payload(payloadSize, '\0');

	std::cout << "PING " << destinationUrl << " from " << appName
			  << " with " << payloadSize << " bytes of data payload." << std::endl;

	for (int i = 0; i < FLAGS_count; ++i) {
		if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			// Something happened on the receiving side, let's stop sending pings.
			break;
		}

		DtnPing message;
		message.set_seq_id(i + 1);
		message.set_data(payload);
    	req.mutable_payload()->PackFrom(message);

		DtnSendResponse resp;
    	grpc::ClientContext context;

		auto startTime = std::chrono::high_resolution_clock::now();
		records.insert({message.seq_id(), {startTime, payloadSize}});
    	grpc::Status status = dtnService->SendBundle(&context, req, &resp);
		if (!status.ok()) {
      		std::cerr << "Error sending bundle (" << status.error_code() << "): " << status.error_message() << std::endl;
      		exit(2);
		}

		if (i != FLAGS_count - 1)
		{
			std::this_thread::sleep_for(delayDuration);
		}
    }

	// Twice the lifetime of a packet past the last one was sent.
	int secondsToWait = FLAGS_lifetime * 2;
	if (fut.wait_for(std::chrono::seconds(secondsToWait)) != std::future_status::ready)
	{
		std::cerr << "Timed out waiting for all responses after " << secondsToWait << " seconds." << std::endl;
		exit(3);
	}

	::grpc::Status result = fut.get();
	// Cancelled as the expected status as the connection is closed by the tool's request.
	if (!result.ok() && (result.error_code() != ::grpc::StatusCode::CANCELLED))
	{
	    std::cerr << "Error while receiving bundles (" << result.error_code()
				  << "): " << result.error_message() << std::endl;
		exit(4);
	}

	return 0;
}

::grpc::Status DtnProtoPing::ReadResponses(dtn::api::DtnService::Stub *dtnService, const std::string& readerGroup)
{
	grpc::ClientContext readContext;

	DtnSubscribeRequest subscribeRequest;
	subscribeRequest.set_group(readerGroup);
	subscribeRequest.set_group_destination(false);

	std::unique_ptr<::grpc::ClientReader<DtnPollResponse>> reader(dtnService->Subscribe(&readContext, subscribeRequest));

	int expectedResponses = FLAGS_count;
	DtnPollResponse resp;
	DtnPing message;
	while (reader->Read(&resp))
	{
		auto responseTime = std::chrono::high_resolution_clock::now();
		resp.payload().UnpackTo(&message);
		auto recordIter = records.find(message.seq_id());
		if (recordIter == records.end())
		{
			std::cout << "Unexpected response with sequence ID " << message.seq_id() << std::endl;
			continue;
		}
		DtnProtoPing::PingRecord record = recordIter->second;
		records.erase(recordIter);

		int bytes = message.data().size();
		if (bytes != record.payloadSize)
		{
			std::cout << "Warning: " << record.payloadSize << " bytes were sent, " << bytes << " returned!" << std::endl;
		}

		std::string timeElapsed = formatTimeElapsed(record.sendTime, responseTime);
		std::cout << bytes << " bytes from " << resp.source_url()
				  << ": seq_id=" << message.seq_id()
				  << " lifetime=" << resp.life_time()
				  << " hops=" << resp.hop_count()
				  << " time=" << timeElapsed << std::endl;
		expectedResponses--;

		if (expectedResponses == 0) {
			// Server should never close the connection willingly so we have to cancel.
			readContext.TryCancel();
		}
	}
	::grpc::Status status = reader->Finish();
	return status;
}

std::string DtnProtoPing::formatTimeElapsed(std::chrono::high_resolution_clock::time_point startTime,
											std::chrono::high_resolution_clock::time_point endTime)
{
	std::ostringstream st;
	auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
	long count = timeElapsed.count();
	if (count < 10000)
	{
		st << count << " ms";
	} else {
		st << static_cast<double>(count) / 1000 << " s";
	}
	return st.str();
}

int main(int argc, char** argv) {
  DtnProtoPing ping;
  ping.Init(&argc, &argv);
  return ping.Execute();
}

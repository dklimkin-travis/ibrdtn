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

#include "protos/dtnservice.grpc.pb.h"
#include "protos/dtnping.pb.h"

#include <thread>
#include <grpc++/grpc++.h>
#include <gflags/gflags.h>

using dtn::api::DtnSendRequest;
using dtn::api::DtnSendResponse;
using dtn::api::DtnService;
using dtn::tools::DtnPing;

DEFINE_string(src, "ping", "Set the source application name");
DEFINE_string(host, "localhost:4592", "gRPC API host and port");
DEFINE_bool(nowait, false, "Do not wait for a reply");
DEFINE_bool(abortfail, false, "Do not wait for a reply");
DEFINE_int32(size, 0, "The size of the payload");
DEFINE_int32(count, 1, "Send <count> echos in a row");
DEFINE_int32(delay, 1, "Delay between echo requests");
DEFINE_int32(lifetime, 30, "Set the lifetime of outgoing bundles");
DEFINE_bool(encrypt, false, "Request encryption on the bundle layer");
DEFINE_bool(sign, false, "Request signature on the bundle layer");

void print_help()
{
	static const char *programName = gflags::ProgramInvocationShortName();
	gflags::ShowUsageWithFlagsRestrict(programName, programName);
}

static const char* getVersionString()
{
	static constexpr char ver[] = PACKAGE_VERSION " (" BUILD_NUMBER ")";
	return ver;
}

static const char* getUsageString()
{
	static constexpr char usage[] = "send echo request over DTN network (IBR-DTN / gRPC API)\n"
			                        "Syntax: dtnprotoping [flags] <dst>\n"
									"Where:\n  <dst> the destination node URL (e.g. dtn://node/echo)";
	return usage;
}

int main(int argc, char **argv)
{
	gflags::SetVersionString(getVersionString());
	gflags::SetUsageMessage(getUsageString());
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	if (argc != 2) {
		print_help();
		return 1;
	}

	auto channel = grpc::CreateChannel(FLAGS_host, grpc::InsecureChannelCredentials());
	std::unique_ptr<DtnService::Stub> stub(DtnService::NewStub(channel));
	auto delayDuration = std::chrono::seconds(FLAGS_delay);

	for (int i = 0; i < FLAGS_count; ++i)
	{
		DtnPing message;
		message.set_seq_id(i + 1);

		DtnSendRequest req;
		req.set_ttl(FLAGS_lifetime);
		req.set_client_id(FLAGS_src);
		req.set_destination_url(argv[1]);
		req.mutable_payload()->PackFrom(message);
		req.set_priority(dtn::api::Priority::NORMAL);

		std::cout << "Sending: " << req.DebugString() << std::endl;

		DtnSendResponse resp;
		grpc::ClientContext context;
		grpc::Status status = stub->SendBundle(&context, req, &resp);

		if (status.ok())
		{
			std::cout << "Ok! : " << resp.DebugString() << std::endl;
		} else
		{
			std::cout << "Error! : " << status.error_code() << ": " << status.error_message() << std::endl;
			return 1;
		}
		std::this_thread::sleep_for(delayDuration);
	}
    return 0;
}

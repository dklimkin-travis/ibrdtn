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

#include "protoapi/ProtoServer.h"

#include "core/BundleCore.h"

#include <ibrcommon/Logger.h>
#include <ibrcommon/net/socket.h>
#include <ibrdtn/data/BundleBuilder.h>

namespace dtn {
namespace api {

const std::string ProtoServer::TAG = "ProtoServer";

ProtoServer::ProtoServer(const std::string& address, const int port)
    : _serverAddress(address + ":" + std::to_string(port)) {}

const std::string ProtoServer::getName() const
{
    return ProtoServer::TAG;
}

void ProtoServer::componentUp() throw()
{
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(_serverAddress, ::grpc::InsecureServerCredentials()).RegisterService(this);
    _server = builder.BuildAndStart();
    if (_server.get() == nullptr) {
	throw ibrcommon::socket_exception("gRPC Server failed to start on: " + _serverAddress);
    }
    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, info) << "gRPC server is running on: " << _serverAddress << IBRCOMMON_LOGGER_ENDL;
}

void ProtoServer::componentRun() throw()
{
    _server->Wait();
    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, info) << "gRPC server is down." << IBRCOMMON_LOGGER_ENDL;
}

void ProtoServer::componentDown() throw()
{
    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, info) << "gRPC server is shutting down." << IBRCOMMON_LOGGER_ENDL;
    _server->Shutdown();
    _server.reset();
}

Status ProtoServer::SendBundle(::grpc::ServerContext* context, const DtnSendRequest* request, DtnSendResponse* reply)
{
    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "Got a protoAPI request:" << IBRCOMMON_LOGGER_ENDL;

    Status res = validateSendRequest(*request);
    if (!res.ok())
    {
		IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "Message validation failed: "
													   << res.error_message() << IBRCOMMON_LOGGER_ENDL;
        return res;
    }

    dtn::data::Bundle bundle;

    ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();
	{   // Block is needed as stream is not closed or updated until ibrcommon::BLOB::iostream is destructed.
		ibrcommon::BLOB::iostream stream = ref.iostream();
	    std::ostream &blobStream = *stream;
		request->payload().SerializeToOstream(&blobStream);
	}
    std::streamsize size = ref.size();
    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "\tPayload of size: " << size << IBRCOMMON_LOGGER_ENDL;
    bundle.push_back(ref);

    bundle.destination = dtn::data::EID(request->destination_url());
    auto srcEid = dtn::core::BundleCore::local;
    srcEid.setApplication(request->client_id());
    bundle.source = srcEid;

    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "\tFrom " << bundle.source.getString() << " to: " << bundle.destination.getString() << IBRCOMMON_LOGGER_ENDL;

    if (!request->custodian_url().empty())
    {
		bundle.custodian = dtn::data::EID(request->custodian_url());
		bundle.set(dtn::data::PrimaryBlock::CUSTODY_REQUESTED, true);
		IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "\tRequest custodian: " << bundle.custodian.getString() << IBRCOMMON_LOGGER_ENDL;
    } else {
		bundle.set(dtn::data::PrimaryBlock::CUSTODY_REQUESTED, false);
    }

    bundle.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, !request->group_destination());
    bundle.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT, request->request_encrypt());
    bundle.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN, request->request_sign());
    bundle.set(dtn::data::PrimaryBlock::IBRDTN_REQUEST_COMPRESSION, request->request_compress());
    bundle.lifetime = request->ttl();
    bundle.setPriority(dtn::data::PrimaryBlock::PRIORITY(request->priority()));

	try
	{
		dtn::core::BundleCore::getInstance().inject(srcEid, bundle, true);
		return Status::OK;
	} catch (const std::exception &e) {
		IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, warning) << "Exception while processing proto-sourced bundle: "
														<< e.what() << IBRCOMMON_LOGGER_ENDL;
		return Status(::grpc::StatusCode::INTERNAL, e.what());
	}
}

Status ProtoServer::validateSendRequest(const DtnSendRequest &request) const
{
    if (request.ttl() <= 0) {
        return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Time-to-live must be positive");
    }
    if (request.priority() > Priority::EXPEDITED) {
        return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Invalid priority value");
    }
	if (request.client_id().empty()) {
		return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Client ID unspecified");
	}
	// Using echo results in self-DOSing of ibrdtn.
	if (request.client_id() == "echo") {
		return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Reserved client ID used");
	}
	if (request.payload().SpaceUsedLong() > MAX_PROTO_MESSAGE_SIZE) {
		return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Message is too large");
	}

#ifndef IBRDTN_SUPPORT_COMPRESSION
	if (request.request_compress()) {
		return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Compression requested but not available");
	}
#endif
#ifndef IBRDTN_SUPPORT_BSP
	if (request.request_encrypt()) {
		return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Encryption requested but not available");
	}
	if (request.request_sign()) {
		return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Signature requested but not available");
	}
#endif

    return Status::OK;
}

Status dtn::api::ProtoServer::PollBundle(
		::grpc::ServerContext *context, const dtn::api::DtnPollRequest *request, dtn::api::DtnPollResponse *response)
{
	dtn::data::EID dstEid = dtn::core::BundleCore::local;
	dstEid.setApplication(request->client_id());

	class SingleBundlePollSelector final : public dtn::storage::BundleSelector
	{
	  public:
		SingleBundlePollSelector(const dtn::data::EID &dst_eid) : _dst_eid(dst_eid) { };
		bool shouldAdd(const dtn::data::MetaBundle& meta) const throw (dtn::storage::BundleSelectorException) override
		{
			if (meta.isFragment()) { return false; }
			return (meta.destination == _dst_eid);
		}
		private:
			const dtn::data::EID& _dst_eid;
	};
	SingleBundlePollSelector selector(dstEid);

	dtn::storage::BundleResultList result;
	try
	{
		dtn::storage::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();
		storage.get(selector, result);
		if (result.empty())
		{
			// This should not happen, but just in case...
			return grpc::Status(::grpc::StatusCode::NOT_FOUND, "Search succeed but no data retrieved.");
		}
	} catch (const dtn::storage::NoBundleFoundException &e)
	{
		// No bundles found, not an error.
		return grpc::Status(::grpc::StatusCode::NOT_FOUND, e.what());
	} catch (const std::exception& e) {
		IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, warning) << "Exception while looking for a bundle: "
														<< e.what() << IBRCOMMON_LOGGER_ENDL;
		return Status(::grpc::StatusCode::INTERNAL, e.what());
	}

	try {
		dtn::data::MetaBundle &meta = result.front();

		// Retrieve useful metadata.
		response->set_source_url(meta.source.getString());
		response->set_hop_count(meta.hopcount.get());

		// Collect payload.
		dtn::data::Bundle bundle = dtn::core::BundleCore::getInstance().getStorage().get(meta);
		ibrcommon::BLOB::Reference ref = bundle.find<dtn::data::PayloadBlock>().getBLOB();
		{
			ibrcommon::BLOB::iostream stream = ref.iostream();
			std::istream &blobStream = *stream;
			response->mutable_payload()->ParseFromIstream(&blobStream);
		}

		response->set_priority(fromBundlePriority(bundle.getPriority()));

		// Bundle is about to be delivered, purge.
		dtn::core::BundlePurgeEvent::raise(meta);
		return Status::OK;
	} catch (const std::exception& e) {
		IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, warning) << "Exception while retrieving a bundle: "
														<< e.what() << IBRCOMMON_LOGGER_ENDL;
		return Status(::grpc::StatusCode::INTERNAL, e.what());
	}
}

dtn::api::Priority ProtoServer::fromBundlePriority(const dtn::data::PrimaryBlock::PRIORITY &rhs)
{
	switch (rhs) {
		case dtn::data::PrimaryBlock::PRIO_HIGH:
			return Priority::EXPEDITED;
		case dtn::data::PrimaryBlock::PRIO_MEDIUM:
			return Priority::NORMAL;
		case dtn::data::PrimaryBlock::PRIO_LOW:
			[[fallthrough]]
		default:
			return Priority::BULK;
	}
}

}  // namespace api
}  // namespace dtn

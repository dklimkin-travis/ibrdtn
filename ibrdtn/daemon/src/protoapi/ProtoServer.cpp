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
#include "core/EventDispatcher.h"
#include "api/ClientHandler.h"

#include <ibrcommon/Logger.h>
#include <ibrcommon/net/socket.h>
#include <ibrdtn/data/BundleBuilder.h>

#include <thread>

namespace dtn {
namespace api {

const std::string ProtoServer::TAG = "ProtoServer";

ProtoServer::ProtoServer(const std::string& address, const int port)
    : _serverAddress(address + ":" + std::to_string(port))
	, _registrator(new RegistrationManager()) { }

const std::string ProtoServer::getName() const
{
    return ProtoServer::TAG;
}

void ProtoServer::componentUp() throw()
{
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(_serverAddress, ::grpc::InsecureServerCredentials()).RegisterService(this);
	builder.SetMaxReceiveMessageSize(MAX_PROTO_MESSAGE_SIZE);
	builder.SetMaxSendMessageSize(MAX_PROTO_MESSAGE_SIZE);
    _server = builder.BuildAndStart();
    if (_server.get() == nullptr) {
		throw ibrcommon::socket_exception("gRPC Server failed to start on: " + _serverAddress);
    }
    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, info) << "gRPC server is running on: " << _serverAddress << IBRCOMMON_LOGGER_ENDL;
}

void ProtoServer::componentRun() throw()
{
	dtn::core::EventDispatcher<dtn::routing::QueueBundleEvent>::add(this);
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
	auto srcEID = generateSourceEID(*request);
    bundle.source = srcEID;

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
    bundle.setPriority(dtn::data::PrimaryBlock::PRIORITY(request->priority() - Priority::BULK));

	bundle.relabel();

	// TODO(dklimkin): what about bundle.relabel();
	// TODO(dklimkin): check address fields for "api:me", this has to be replaced

	try
	{
		dtn::core::BundleCore::getInstance().inject(srcEID, bundle, true);
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
	// Using echo results in self-DOSing of ibrdtn.
	if (request.client_id() == "echo") {
		return Status(::grpc::StatusCode::INVALID_ARGUMENT, "Reserved client ID used");
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

Status dtn::api::ProtoServer::PollBundle(::grpc::ServerContext *context, const DtnPollRequest *request, DtnPollResponse *response)
{
	// TODO(dklimkin): check for existing registrations first.

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
			return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Search succeed but no data retrieved.");
		}
	} catch (const dtn::storage::NoBundleFoundException &e)
	{
		// No bundles found, not an error.
		return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, e.what());
	} catch (const std::exception& e) {
		IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, warning) << "Exception while looking for a bundle: "
														<< e.what() << IBRCOMMON_LOGGER_ENDL;
		return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
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
		// TODO(dklimkin): this is not good enough, should raise event AFTER delivery.
		dtn::core::BundleEvent::raise(meta, dtn::core::BUNDLE_DELIVERED);
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
			return Priority::BULK;
		default:
			return Priority::UNSPECIFIED;
	}
}

Status ProtoServer::Subscribe(::grpc::ServerContext* context, const DtnSubscribeRequest* request, DtnWriter* writer)
{
	const std::string peerId = context->peer();
	const std::string groupName = request->group();
    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "Client " << peerId << " requested subscription to: "
												   << groupName << IBRCOMMON_LOGGER_ENDL;
	ProtoConnection conn(groupName, context, writer);
	Status result = _registrator->AddRegistration(groupName, request->group_destination(), peerId, &conn);
	if (result.ok())
	{
		result = conn.RunConnection();
	}
	_registrator->RemoveRegistrations(peerId);
	return result;
}

void ProtoServer::raiseEvent(const dtn::routing::QueueBundleEvent &queued) throw ()
{
	// ignore fragments - we can not deliver them directly to the client.
	if (queued.bundle.isFragment()) return;

	dtn::data::EID destination = queued.bundle.destination;

    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "Event for new bundle destined for: "
												   << destination.getString() << IBRCOMMON_LOGGER_ENDL;
	std::string destGroup = destination.getApplication();

	int deliveries = _registrator->SendToGroup(destGroup, queued.bundle);
/*
	if (deliveries > 0)
	{
	    IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "Bundle delivered to " << deliveries << " subscriber(s)" << IBRCOMMON_LOGGER_ENDL;
		dtn::core::BundleEvent::raise(queued.bundle, dtn::core::BUNDLE_DELIVERED);
		dtn::core::BundlePurgeEvent::raise(queued.bundle);
	} else {
		IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, notice) << "Not proto subscribers to deliver too." << IBRCOMMON_LOGGER_ENDL;
	}
 */
}

::grpc::Status ProtoServer::pushBundle(DtnWriter *writer, const data::MetaBundle& meta)
{
	try {
		DtnPollResponse response;

		// Retrieve useful metadata.
		response.set_source_url(meta.source.getString());
		response.set_hop_count(meta.hopcount.get());

		// Collect payload.
		dtn::data::Bundle bundle = dtn::core::BundleCore::getInstance().getStorage().get(meta);
		ibrcommon::BLOB::Reference ref = bundle.find<dtn::data::PayloadBlock>().getBLOB();
		{
			ibrcommon::BLOB::iostream stream = ref.iostream();
			std::istream &blobStream = *stream;
			response.mutable_payload()->ParseFromIstream(&blobStream);
		}

		response.set_priority(fromBundlePriority(bundle.getPriority()));
		writer->Write(response);
		return Status::OK;
	} catch (const std::exception& e) {
		IBRCOMMON_LOGGER_TAG(ProtoServer::TAG, warning) << "Exception while retrieving a bundle: "
														<< e.what() << IBRCOMMON_LOGGER_ENDL;
		return Status(::grpc::StatusCode::INTERNAL, e.what());
	}
}

dtn::data::EID ProtoServer::generateSourceEID(const DtnSendRequest& request)
{
	std::string appId = request.client_id();
	if (appId.empty())
	{
		appId = dtn::utils::Random::gen_chars(16);
	}
	dtn::data::EID result = dtn::core::BundleCore::local;
	result.setApplication(appId);
	return result;
}

}  // namespace api
}  // namespace dtn

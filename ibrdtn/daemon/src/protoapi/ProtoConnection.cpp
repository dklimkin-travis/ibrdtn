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

#include "protoapi/ProtoConnection.h"

#include "core/BundleCore.h"

#include <ibrcommon/Logger.h>


namespace dtn
{
namespace api
{

const std::string ProtoConnection::TAG = "ProtoConnection";

ProtoConnection::ProtoConnection(const std::string& groupName, ::grpc::ServerContext *context, DtnWriter *writer)
	: _hasBundles(false), _shutdown(false), _context(context), _writer(writer), _peerEID(createEID(groupName))
{
	std::cout << "Created ProtoConnection" << std::endl;
}

ProtoConnection::~ProtoConnection()
{
	std::cout << "Destroyed ProtoConnection" << std::endl;
}

const dtn::data::EID ProtoConnection::createEID(const std::string &groupName) const
{
	dtn::data::EID eid = dtn::core::BundleCore::local;
	eid.setApplication(groupName);
	return eid;
}

::grpc::Status ProtoConnection::RunConnection() {
	while (!_shutdown)
	{
		if (_context->IsCancelled()) break;

		std::unique_lock<std::mutex> lk(_mutex);
		_queueCond.wait(lk, [this] { return (_shutdown || _hasBundles); });
		_hasBundles = false;
		lk.unlock();

		if (_shutdown) break;

		::grpc::Status result = sendBundles();
		if (!result.ok())
		{
			return result;
		}
  	}
  	return ::grpc::Status::OK;
}

void ProtoConnection::PushBundle(const data::MetaBundle &bundle) {
	// We don't really know what this reference points too with IBR, nor does
	// the author. So we merely just look for any bundles in the connection thread.
	std::lock_guard<std::mutex> l(_mutex);
	_hasBundles = true;
	_queueCond.notify_one();
}

void ProtoConnection::Shutdown()
{
	std::lock_guard<std::mutex> l(_mutex);
	_shutdown = true;
	_queueCond.notify_one();
}

::grpc::Status ProtoConnection::sendBundles()
{
	std::cout << "*** Entering ITERATION" << std::endl;

	GroupBundlePollSelector selector(_peerEID);

	dtn::storage::BundleResultList result;
	try
	{
		dtn::storage::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();
		storage.get(selector, result);
	}
	catch (const dtn::storage::NoBundleFoundException &e)
	{
		// No bundles found, not an error.
		return ::grpc::Status::OK;
	} catch (const std::exception& e) {
		IBRCOMMON_LOGGER_TAG(TAG, warning) << "Exception while looking for a bundle: " << e.what() << IBRCOMMON_LOGGER_ENDL;
		return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
	}

	try
	{
		for (dtn::data::MetaBundle &meta : result)
		{
			DtnPollResponse response;

			// Retrieve useful metadata.
			response.set_source_url(meta.source.getString());
			response.set_hop_count(meta.hopcount.get());
			response.set_priority(fromMetaPriority(meta.getPriority()));
			// TODO(dklimkin): maybe collect advanced flags.

			IBRCOMMON_LOGGER_TAG(TAG, warning) << "Attempting to deliver bundle: " << meta.toString() << " from "
						<< meta.source.getString() << " to " << meta.destination.getString() << IBRCOMMON_LOGGER_ENDL;

			// Collect payload.
			dtn::data::Bundle bundle = dtn::core::BundleCore::getInstance().getStorage().get(meta);
			ibrcommon::BLOB::Reference ref = bundle.find<dtn::data::PayloadBlock>().getBLOB();
			{
				ibrcommon::BLOB::iostream stream = ref.iostream();
				std::istream &blobStream = *stream;
				response.mutable_payload()->ParseFromIstream(&blobStream);
			}

			if (_writer->Write(response))
			{
				IBRCOMMON_LOGGER_TAG(TAG, warning) << "... delivered bundle: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;
				dtn::core::BundleEvent::raise(meta, dtn::core::BUNDLE_DELIVERED);
				// Bundle was delivered, purge.
				dtn::core::BundlePurgeEvent::raise(meta);
				IBRCOMMON_LOGGER_TAG(TAG, warning) << "... removed bundle: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;
			} else
			{
				_shutdown = true;
				break;
			}
		}
	} catch (const dtn::storage::NoBundleFoundException &e) {
		IBRCOMMON_LOGGER_TAG(TAG, warning) << "Listed bundle is gone during iteration: "
										   << e.what() << IBRCOMMON_LOGGER_ENDL;
		std::cout << "*** Exiting ITERATION" << std::endl;
		return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
	} catch (const std::exception& e) {
		IBRCOMMON_LOGGER_TAG(TAG, warning) << "Exception while retrieving a bundle: "
											<< e.what() << IBRCOMMON_LOGGER_ENDL;
		std::cout << "*** Exiting ITERATION" << std::endl;
		return ::grpc::Status(::grpc::StatusCode::INTERNAL, e.what());
	}

	std::cout << "*** Exiting ITERATION" << std::endl;

	return ::grpc::Status::OK;
}

dtn::api::Priority ProtoConnection::fromMetaPriority(const int priority)
{
	switch (priority)
	{
	case 2:
		return Priority::EXPEDITED;
	case 1:
		return Priority::NORMAL;
	case 0:
		return Priority::BULK;
	default:
		return Priority::UNSPECIFIED;
	}
}

}  // namespace api
}  // namespace dtn

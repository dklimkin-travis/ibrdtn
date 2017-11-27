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

#ifndef IBRDTN_DAEMON_SRC_PROTOAPI_PROTO_CONNECTION_H_
#define IBRDTN_DAEMON_SRC_PROTOAPI_PROTO_CONNECTION_H_

#include "protos/dtnservice.grpc.pb.h"

#include "storage/BundleSelector.h"

#include <ibrdtn/data/MetaBundle.h>

#include <grpc++/grpc++.h>
#include <condition_variable>
#include <atomic>

namespace dtn
{
namespace api
{

typedef ::grpc::ServerWriter<DtnPollResponse> DtnWriter;

class ProtoConnection final
{
public:
	explicit ProtoConnection(const std::string& groupName, ::grpc::ServerContext *context, DtnWriter *writer);
	~ProtoConnection();

	// ProtoConnection is neither copyable nor movable.
	ProtoConnection(const ProtoConnection &) = delete;
	ProtoConnection &operator=(const ProtoConnection &) = delete;

	::grpc::Status RunConnection();

	void PushBundle(const data::MetaBundle& bundle);

	void Shutdown();
private:
	class GroupBundlePollSelector final : public dtn::storage::BundleSelector
	{
	  public:
		GroupBundlePollSelector(const dtn::data::EID &dst_eid) : _dst_eid(dst_eid) { };
		dtn::data::Size limit() const throw () { return MAX_RESPONSE_BUNDLES; };
		bool shouldAdd(const dtn::data::MetaBundle& meta) const throw (dtn::storage::BundleSelectorException) override
		{
			if (meta.isFragment()) { return false; }
			return (meta.destination == _dst_eid);
		}
		private:
			const dtn::data::EID& _dst_eid;
	};

	static const std::string TAG;
	static constexpr const dtn::data::Size MAX_RESPONSE_BUNDLES = 1000;

	std::mutex _mutex;
	std::condition_variable _queueCond;
	bool _hasBundles;
	std::atomic<bool> _shutdown;

	::grpc::ServerContext* _context;
	DtnWriter* _writer;

	const dtn::data::EID _peerEID;

	const dtn::data::EID createEID(const std::string& groupName) const;

	::grpc::Status sendBundles();
	static dtn::api::Priority fromMetaPriority(const int priority);
};

}  // namespace api
}  // namespace dtn

#endif  // IBRDTN_DAEMON_SRC_PROTOAPI_PROTO_CONNECTION_H_

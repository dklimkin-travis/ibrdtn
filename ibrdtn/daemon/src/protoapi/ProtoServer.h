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

#ifndef IBRDTN_DAEMON_SRC_API_PROTO_CLIENT_H_
#define IBRDTN_DAEMON_SRC_API_PROTO_CLIENT_H_

#include "protos/dtnservice.grpc.pb.h"

#include "Component.h"

#include <ibrdtn/data/PrimaryBlock.h>

#include <grpc++/grpc++.h>
#include <string>

using ::grpc::Status;

namespace dtn
{
    namespace api
    {
	class ProtoServer final : public dtn::api::DtnService::Service, public dtn::daemon::IndependentComponent
	{
	public:
	    ProtoServer(const std::string& address, const int port);

	    // ProtoServer is neither copyable nor movable.
	    ProtoServer(const ProtoServer &) = delete;
	    ProtoServer &operator=(const ProtoServer &) = delete;

	    // TODO: should be movable going forward.

	    virtual const std::string getName() const override;
	    virtual void __cancellation() throw () {};
	    virtual void componentUp() throw ();
	    virtual void componentRun() throw ();
	    virtual void componentDown() throw ();

	    Status SendBundle(::grpc::ServerContext* context, const DtnSendRequest* request, DtnSendResponse* reply) override;
		Status PollBundle(::grpc::ServerContext* context, const DtnPollRequest* request, DtnPollResponse* response) override;

	private:
	    static const std::string TAG;
		static const size_t MAX_PROTO_MESSAGE_SIZE = 64*1024*1024; 	// 64Mb

	    const std::string _serverAddress;
	    std::unique_ptr<::grpc::Server> _server;

		Status validateSendRequest(const DtnSendRequest& request) const;

		static dtn::api::Priority fromBundlePriority(const dtn::data::PrimaryBlock::PRIORITY &rhs);
	};

} // namespace api
} // namespace dtn

#endif  // IBRDTN_DAEMON_SRC_API_PROTO_CLIENT_H_

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

#ifndef IBRDTN_DAEMON_SRC_API_PROTOAPI_REGISTARTION_MANAGER_H_
#define IBRDTN_DAEMON_SRC_API_PROTOAPI_REGISTARTION_MANAGER_H_

#include "protos/dtnservice.grpc.pb.h"

#include "protoapi/ProtoConnection.h"

#include <bits/unordered_map.h>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>

namespace dtn {
namespace api {

typedef ::grpc::ServerWriter<DtnPollResponse> DtnWriter;

class RegistrationManager final
{
public:
	RegistrationManager() = default;

	// RegistrationManager is neither copyable nor movable.
	RegistrationManager(const RegistrationManager &) = delete;
	RegistrationManager &operator=(const RegistrationManager &) = delete;

	::grpc::Status AddRegistration(const std::string& groupName, bool isGroup,
								   const std::string& peerId, ProtoConnection *conn);

	int SendToGroup(const std::string& groupName, const data::MetaBundle& meta);
	void RemoveRegistrations(const std::string& peerId);

private:
	struct Registrant {
		ProtoConnection *conn;
		std::unordered_set<std::string> groups;
	};
	struct Entry {
		bool isGroup;
		std::string name;
		std::unordered_set<std::string> subscribers;
	};

	std::string TAG = "RegistrationManager";

	std::map<std::string, std::unique_ptr<Registrant>> _registrants;
	std::unordered_map<std::string, std::unique_ptr<Entry>> _groups;

	std::mutex registryMutex;
};


}  // namespace api
}  // namespace dtn

#endif  // IBRDTN_DAEMON_SRC_API_PROTOAPI_REGISTARTION_MANAGER_H_

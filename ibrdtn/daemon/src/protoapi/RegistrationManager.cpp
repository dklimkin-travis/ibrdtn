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

#include "protoapi/RegistrationManager.h"

#include <ibrcommon/Logger.h>

#include <memory>

namespace dtn {
namespace api {

::grpc::Status RegistrationManager::AddRegistration(const std::string& groupName, bool isGroup, const std::string& peerId, ProtoConnection *conn)
{
	std::lock_guard<std::mutex> lock(registryMutex);

	auto iterPeer = _registrants.find(peerId);
	if (iterPeer == _registrants.end()) {
    	IBRCOMMON_LOGGER_TAG(RegistrationManager::TAG, notice) << "New registration: " << peerId << IBRCOMMON_LOGGER_ENDL;
		std::unique_ptr<Registrant> newPeer(new Registrant());
		newPeer->conn = conn;
		newPeer->groups.insert(groupName);
		_registrants[peerId] = std::move(newPeer);
	} else {
    	IBRCOMMON_LOGGER_TAG(RegistrationManager::TAG, notice) << "Update registration for: " << peerId << IBRCOMMON_LOGGER_ENDL;
		std::unique_ptr<Registrant>& reg = iterPeer->second;
		reg->groups.insert(groupName);
	}

	auto iterGroup = _groups.find(groupName);
	if (iterGroup == _groups.end())
	{
		// No one is registered for the group, creating new one.
	   	IBRCOMMON_LOGGER_TAG(RegistrationManager::TAG, notice) << "New group: " << groupName << IBRCOMMON_LOGGER_ENDL;
		std::unique_ptr<Entry> newGroup(new Entry);
		newGroup->isGroup = isGroup;
		newGroup->name = groupName;
		newGroup->subscribers.insert(peerId);
		_groups[groupName] = std::move(newGroup);
	} else {
		std::unique_ptr<Entry>& group = iterGroup->second;
		if (isGroup && group->isGroup) {
			// Subscribing to the existing group.
    		IBRCOMMON_LOGGER_TAG(RegistrationManager::TAG, notice) << "New group: " << groupName << IBRCOMMON_LOGGER_ENDL;
			group->subscribers.insert(peerId);
		} else {
			if (isGroup) {
				IBRCOMMON_LOGGER_TAG(RegistrationManager::TAG, notice)
					<< "Unable to exclusively join existing group: " << groupName << IBRCOMMON_LOGGER_ENDL;
			} else {
				IBRCOMMON_LOGGER_TAG(RegistrationManager::TAG, notice)
					<< "Unable to join existing exclusive group: " << groupName << IBRCOMMON_LOGGER_ENDL;
			}
	    	return ::grpc::Status(::grpc::StatusCode::ALREADY_EXISTS, "Group " + groupName + " is already registered.");
		}
	}

	return ::grpc::Status::OK;
}

int RegistrationManager::SendToGroup(const std::string& groupName, const data::MetaBundle& meta)
{
	std::lock_guard<std::mutex> lock(registryMutex);

	auto iter = _groups.find(groupName);
	if (iter == _groups.end()) {
		return false;
	}

	int deliveries = 0;

	for (const std::string& peer : iter->second->subscribers) {
		auto iterReg = _registrants.find(peer);
		if (iterReg == _registrants.end())
		{
			continue;
		}
		std::unique_ptr<Registrant>& reg = iterReg->second;
	    IBRCOMMON_LOGGER_TAG(RegistrationManager::TAG, notice) << "\tPeer " << peer << " is subscribed to group "
					<< groupName << ", forwarding." << IBRCOMMON_LOGGER_ENDL;
		reg->conn->PushBundle(meta);
		++deliveries;
	}
	return deliveries;
}

void RegistrationManager::RemoveRegistrations(const std::string& peerId)
{
	std::lock_guard<std::mutex> lock(registryMutex);

	auto iterPeer = _registrants.find(peerId);
	if (iterPeer != _registrants.end())
	{
		auto& peer = iterPeer->second;
		peer->conn->Shutdown();

		for (auto& groupName : peer->groups)
		{
			auto iterGroup = _groups.find(groupName);
			if (iterGroup != _groups.end())
			{
				auto &group = iterGroup->second;
				group->subscribers.erase(peerId);
				if (group->subscribers.empty()) {
		    		IBRCOMMON_LOGGER_TAG(RegistrationManager::TAG, notice)
						<< "Group " << groupName << " is empty, removing." << IBRCOMMON_LOGGER_ENDL;
					_groups.erase(iterGroup);
				}
			}
		}
		_registrants.erase(iterPeer);
	}
}

}  // namespace api
}  // namespace dtn

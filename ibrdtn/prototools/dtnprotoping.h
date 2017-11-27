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

#ifndef IBRDTN_PROTOTOOLS_DTNPROTOPING_H_
#define IBRDTN_PROTOTOOLS_DTNPROTOPING_H_

#include "dtnprototool.h"

#include "protos/dtnservice.grpc.pb.h"

#include <grpc++/grpc++.h>

using dtn::api::DtnPollResponse;

class DtnProtoPing final: private DtnProtoTool
{
public:
	DtnProtoPing() = default;
	virtual ~DtnProtoPing() = default;

    virtual void Init(int* argc, char*** argv) override;
	virtual int Execute() override;

protected:
	virtual const char* GetUsageString() override;
	static std::string formatTimeElapsed(std::chrono::high_resolution_clock::time_point startTime,
								  		 std::chrono::high_resolution_clock::time_point endTime);

private:
	struct PingRecord
	{
		std::chrono::high_resolution_clock::time_point sendTime;
		int payloadSize;
	};

	std::string destinationUrl;
	std::map<int, PingRecord> records;

	::grpc::Status ReadResponses(dtn::api::DtnService::Stub *dtnService, const std::string& readerGroup);
};

#endif // IBRDTN_PROTOTOOLS_DTNPROTOTTOL_H_

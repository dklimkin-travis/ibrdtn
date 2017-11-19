#include <iostream>

#include "protos/dtnservice.grpc.pb.h"
#include <grpc++/grpc++.h>

using dtn::api::DtnPollRequest;
using dtn::api::DtnPollResponse;
using dtn::api::DtnService;

int main() {
/*    auto channel = grpc::CreateChannel("localhost:4592", grpc::InsecureChannelCredentials());
    std::unique_ptr<DtnService::Stub> stub(DtnService::NewStub(channel));
    grpc::ClientContext context;

    DtnPollResponse resp;
    DtnPollRequest request;
    request.set_client_id("ping");
    grpc::Status status = stub->PollBundle(&context, request, &resp);

    if (status.ok()) {
        std::cout << "Got bundle:\n" << resp.DebugString() << std::endl;
        DtnMessage msg;
        resp.payload().UnpackTo(&msg);
        std::cout << "Message: " << msg.message() << std::endl;
    } else {
        if (status.error_code() == grpc::StatusCode::NOT_FOUND) {
            std::cout << "No bundles waiting: " << status.error_message() << std::endl;
        } else {
            std::cout << "Error occurred: " << status.error_message() << std::endl;
            return 1;
        }
    }*/
    return 0;
}

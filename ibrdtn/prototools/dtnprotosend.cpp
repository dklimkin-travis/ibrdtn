#include <iostream>

#include "protos/dtnservice.grpc.pb.h"
#include <grpc++/grpc++.h>

using dtn::api::DtnSendRequest;
using dtn::api::DtnSendResponse;
using dtn::api::DtnService;

int main() {
/*    static int seq = 1;

    DtnMessage message;
    message.set_message("Hello World!");
    message.set_seq_id(seq++);

    DtnSendRequest req;

    req.set_ttl(3600);
    req.set_client_id("ping");
    req.set_destination_url("dtn://murzik/echo");
    req.mutable_payload()->PackFrom(message);
    req.set_priority(dtn::api::Priority::EXPEDITED);

    std::cout << req.DebugString() << std::endl;

    auto channel = grpc::CreateChannel("localhost:4592", grpc::InsecureChannelCredentials());
    std::unique_ptr<DtnService::Stub> stub(DtnService::NewStub(channel));
    grpc::ClientContext context;

    DtnSendResponse resp;
    grpc::Status status = stub->SendBundle(&context, req, &resp);

    if (status.ok()) {
        std::cout << "Ok! : " << resp.DebugString() << std::endl;
    } else {
        std::cout << "Error! : " << status.error_code() << ": " << status.error_message() << std::endl;
    }*/
    return 0;
}
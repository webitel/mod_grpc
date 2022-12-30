//
// Created by root on 30.12.22.
//

#ifndef MOD_GRPC_AMD_CLIENT_H
#define MOD_GRPC_AMD_CLIENT_H

#include <thread>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "generated/fs.grpc.pb.h"
#include "generated/stream.grpc.pb.h"

class AsyncClientCall {
public:
    // Container for the data we expect from the server.
    ::amd::StreamPCMResponse reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // Storage for the status of the RPC upon completion.
    grpc::Status status;

    std::unique_ptr<grpc::ClientAsyncWriter<::amd::StreamPCMRequest>> writer;
};

class AMDClient {
public:
    explicit AMDClient(std::shared_ptr<grpc::Channel> channel)
            : stub_(::amd::Api::NewStub(channel)) {
        thread_ = std::thread(&AMDClient::AsyncCompleteRpc, this);
    }

    ~AMDClient() {
        cq_.Shutdown();
        stub_.reset();
        thread_.join();
    }



    AsyncClientCall* Stream(int64_t domain_id, const char *uuid, const char *name, int32_t rate) {
        AsyncClientCall* call = new AsyncClientCall;

        ::amd::StreamPCMRequest msg;
        auto metadata = msg.mutable_metadata();
        metadata->set_uuid(uuid);
        metadata->set_name(name);
        metadata->set_domain_id(domain_id);
        metadata->set_mime_type("audio/pcma");
        metadata->set_sample_rate(rate);

        writer = stub_->PrepareAsyncStreamPCM(&call->context, &call->reply, &cq_);
        writer->Write(msg, call);

        return call;
    }

    // Loop while listening for completed responses.
    // Prints out the response from the server.
    void AsyncCompleteRpc() {
        void* got_tag;
        bool ok = false;

        // Block until the next result is available in the completion queue "cq".
        while (cq_.Next(&got_tag, &ok)) {
            // The tag in this example is the memory location of the call object
            AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            GPR_ASSERT(ok);

            if (call->status.ok())
                std::cout << "Greeter received: " << call->reply.result() << std::endl;
            else
                std::cout << "RPC failed" << std::endl;

            // Once we're complete, deallocate the call object.
            delete call;
        }
    }

private:
    std::unique_ptr< ::grpc::ClientAsyncWriter< ::amd::StreamPCMRequest>> writer;
    std::unique_ptr<::amd::Api::Stub> stub_;
    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    grpc::CompletionQueue cq_;
    std::thread thread_;
};


#endif //MOD_GRPC_AMD_CLIENT_H

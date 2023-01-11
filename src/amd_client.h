//
// Created by root on 30.12.22.
//

#ifndef MOD_GRPC_AMD_CLIENT_H
#define MOD_GRPC_AMD_CLIENT_H

extern "C" {
#include <switch.h>
}

#include <condition_variable>
#include <thread>
#include <queue>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include <mutex>
#include <shared_mutex>

#include "generated/fs.grpc.pb.h"
#include "generated/stream.grpc.pb.h"

class AsyncClientCall {
public:
    void Listen() {
        rt = std::thread([this] {
            rw->Read(&this->reply);
            // mutex ?
            dataReady = true;
        });

    };

    bool Finished() const {
        return dataReady;
    }

    bool Finish() {
        rw->WritesDone();
        rw->Finish();
        if (rt.joinable()) {
            rt.join();
        }
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AsyncClientCall::Finish\n");
        return true;
    }

    bool Write(void *data, size_t len) const {
        ::amd::StreamPCMRequest msg;
        msg.set_chunk(data, len);
        return rw->Write(msg);
    }

    bool Write(uint8_t *data, size_t len) {
        ::amd::StreamPCMRequest msg;
        msg.set_chunk(data, len);
        return rw->Write(msg);
    }

    ~AsyncClientCall() {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Destroy AsyncClientCall\n");
    }

    ::amd::StreamPCMResponse reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;
    std::thread rt;

    bool dataReady = false;
    std::unique_ptr<::grpc::ClientReaderWriter<::amd::StreamPCMRequest, amd::StreamPCMResponse>> rw;
};

class AMDClient {
public:
    explicit AMDClient(std::shared_ptr<grpc::Channel> channel)
            : stub_(::amd::Api::NewStub(channel)) {
    }

    ~AMDClient() {
        stub_.reset();
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Destroy AMDClient\n");
    }


    AsyncClientCall *Stream(int64_t domain_id, const char *uuid, const char *name, int32_t rate) {
        //todo
        auto *call = new AsyncClientCall();

        ::amd::StreamPCMRequest msg;
        auto metadata = msg.mutable_metadata();
        metadata->set_uuid(uuid);
        metadata->set_name(name);
        metadata->set_domain_id(domain_id);
        metadata->set_mime_type("audio/pcma");
        metadata->set_sample_rate(rate);

        call->rw = stub_->StreamPCM(&call->context);

        if (!call->rw->Write(msg)) {
            delete call;
            return nullptr;
        };

        call->Listen();

        return call;
    }

private:
    std::unique_ptr<::amd::Api::Stub> stub_;
};


#endif //MOD_GRPC_AMD_CLIENT_H

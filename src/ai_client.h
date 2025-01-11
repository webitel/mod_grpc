//
// Created by root on 09.01.25.
//

#ifndef MOD_GRPC_AI_CLIENT_H
#define MOD_GRPC_AI_CLIENT_H


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
#include <fstream>

#include "generated/fs.grpc.pb.h"
#include "generated/voicebot.grpc.pb.h"

class AiClientCall {
public:
    void Listen() {
        rt = std::thread([this] {
            ::voicebot::AudioResponse reply;
            switch_audio_resampler_t *resampler;
            switch_resample_create(&resampler,
                                   16000,
                                   8000,
                                   320, SWITCH_RESAMPLE_QUALITY, 1);

            while (rw->Read(&reply)) {
                const auto& chunk = reply.audio_data();
                size_t input_samples = chunk.size();
                int16_t input_buffer[input_samples];
                memcpy(input_buffer, chunk.data(), chunk.size());
                switch_resample_process(resampler, input_buffer, input_samples / 2);
                memcpy(input_buffer, resampler->to, resampler->to_len * 2 );
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "rec %ld\n", chunk.size());

//                fout.write(chunk.data(), chunk.size());


                if (resampler->to_len > 0) {
                    switch_buffer_lock(buffer);
                    switch_buffer_write(buffer, input_buffer, resampler->to_len * 2);
                    switch_buffer_unlock(buffer);

                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AiClientCall::READ - Resampled and written to buffer\n");
                } else {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Resample process failed\n");
                }

            }

            switch_resample_destroy(&resampler);
        });
    };

    bool Finish() {
        rw->WritesDone();
        rw->Finish();
        if (rt.joinable()) {
            rt.join();
        }
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AiClientCall::Finish\n");
        return true;
    }

    inline bool write(void *data, uint32_t datalen)  {
        audio_buffer.insert(audio_buffer.end(), (uint8_t*)data, (uint8_t*)data + datalen);
        size_t target_frame_size = 1024;
        bool ok(true);
        while (audio_buffer.size() >= target_frame_size) {
            std::vector<uint8_t> send_buffer(audio_buffer.begin(), audio_buffer.begin() + target_frame_size);
            request.clear_audio_data();
            request.set_audio_data(send_buffer.data(), send_buffer.size());
            request.set_conversation_id(id);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AiClientCall::Write\n");
            ok = rw->Write(request);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AiClientCall::WriteEnd\n");
            audio_buffer.erase(audio_buffer.begin(), audio_buffer.begin() + target_frame_size);
        }
        return ok;
    }

    bool Write(void *data, uint32_t len) {
        return this->write(data, len);
    }

    inline bool Write(uint8_t *data, uint32_t len) {
        return this->write(data, (uint32_t)len);
    }

    ~AiClientCall() {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Destroy AiClientCall\n");
    }

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;
    std::thread rt;
    switch_buffer_t *buffer = nullptr;
    switch_mutex_t *mutex;
    std::vector<uint8_t> audio_buffer;
    voicebot::AudioRequest request;
    std::string id;

    std::unique_ptr<::grpc::ClientReaderWriter<::voicebot::AudioRequest, voicebot::AudioResponse>> rw;
};

class AiClient {
public:
    explicit AiClient(const std::shared_ptr<grpc::Channel>& channel)
            : stub_(::voicebot::VoiceBot::NewStub(channel)) {
    }

    ~AiClient() {
        stub_.reset();
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Destroy AiClient\n");
    }


    AiClientCall *Stream(int64_t domain_id, const char *uuid, const char *name, int32_t rate) {
        //todo
        auto *call = new AiClientCall();
        call->rw = stub_->Converse(&call->context);
        call->id = std::string(name);

        ::voicebot::AudioRequest msg;
        msg.set_conversation_id(uuid);

        if (!call->rw->Write(msg)) {
            delete call;
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Exit\n");
            return nullptr;
        };

        return call;
    }

private:
    std::unique_ptr<::voicebot::VoiceBot::Stub> stub_;
};

#endif //MOD_GRPC_AI_CLIENT_H

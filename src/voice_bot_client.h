//
// Created by root on 09.01.25.
//

#ifndef MOD_GRPC_VOICE_BOT_CLIENT_H
#define MOD_GRPC_VOICE_BOT_CLIENT_H


extern "C" {
#include <switch.h>
}

#include <condition_variable>
#include <thread>
#include <queue>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include <fstream>
#include <utility>

#include "generated/fs.grpc.pb.h"
#include "generated/voicebot.grpc.pb.h"

class VoiceBotCall {
public:
    explicit VoiceBotCall(std::string callId, int32_t model_rate_, int32_t channel_rate_,
                          std::unique_ptr<::voicebot::VoiceBot::Stub> &stub_) {
        model_rate = model_rate_;
        channel_rate = channel_rate_;
        id = std::move(callId);
        rw = stub_->Converse(&context);
    }

    bool Start(std::string &start_message) {
        ::voicebot::AudioRequest req;
        auto metadata = req.mutable_metadata();
        metadata->set_conversation_id(id);
        metadata->set_rate(model_rate);
        if (!start_message.empty()) {
            metadata->set_initial_ai_message(start_message);
        }

        return rw->Write(req);
    }

    void Listen() {
        rt = std::thread([this] {
            ::voicebot::AudioResponse reply;
            switch_audio_resampler_t *resampler = nullptr;
            int out_rate = 16000;
            if (out_rate != channel_rate) {
                switch_resample_create(&resampler,
                                       out_rate,
                                       channel_rate,
                                       320, SWITCH_RESAMPLE_QUALITY, 1);
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "resample reader from %d to %d\n", out_rate, channel_rate);
            }

            while (rw->Read(&reply)) {
                const auto &chunk = reply.audio_data();
                static const int MAX_CHUNK_SIZE = 13*8024;

                if (chunk.size() > MAX_CHUNK_SIZE) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "skip chunk %ld, size > %d\n",
                                      chunk.size(), MAX_CHUNK_SIZE);
                    continue;
                }

                if (!chunk.empty()) {
                    size_t input_samples = chunk.size();
                    int16_t input_buffer[input_samples];
                    memcpy(input_buffer, chunk.data(), chunk.size());

                    if (resampler) {
                        switch_resample_process(resampler, input_buffer, input_samples / 2);
                        memcpy(input_buffer, resampler->to, resampler->to_len * 2);
                        input_samples = resampler->to_len * 2;
                    }

                    if (input_samples) {
                        switch_buffer_lock(buffer);
                        switch_buffer_write(buffer, input_buffer, input_samples);
                        switch_buffer_unlock(buffer);
                    }

                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "receive chunk %ld\n",
                                      chunk.size());
                }

                if (reply.end_conversation()) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "receive end conversation\n");
                    break;
                } else if (reply.stop_talk()) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "receive stop\n");
                    switch_buffer_lock(buffer);
                    if (switch_buffer_inuse(buffer)) {
                        switch_buffer_zero(buffer);
                    }
                    switch_buffer_unlock(buffer);
                }
            }
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "close reader\n");

            if (resampler) {
                switch_resample_destroy(&resampler);
            }
            setBreak();
        });
    };

    bool Finish() {

        context.TryCancel();
        rw->WritesDone();
        auto status = rw->Finish();
        if (!status.ok()) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "error: %s [%s]\n",
                              status.error_message().c_str(), status.error_details().c_str());
        }
        if (rt.joinable()) {
            rt.join();
        }
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "finish\n");
        return true;
    }

    inline bool write(void *data, uint32_t datalen) {
        audio_buffer.insert(audio_buffer.end(), (uint8_t *) data, (uint8_t *) data + datalen);
        size_t target_frame_size = 1024;
        if (model_rate == 8000) {
            target_frame_size = 512;
        }
        bool ok(true);
        while (audio_buffer.size() >= target_frame_size) {
            std::vector<uint8_t> send_buffer(audio_buffer.begin(), audio_buffer.begin() + target_frame_size);
            ::voicebot::AudioRequest req;
            auto audio = req.mutable_audiodata();
            audio->set_conversation_id(id);
            audio->set_audio_bytes(send_buffer.data(), send_buffer.size());
//            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AiClientCall::Write\n");
            ok = rw->Write(req);
//            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "AiClientCall::WriteEnd\n");
            audio_buffer.erase(audio_buffer.begin(), audio_buffer.begin() + target_frame_size);
        }
        return ok;
    }

    inline bool Write(void *data, uint32_t len) {
        return this->write(data, len);
    }

    inline bool Write(uint8_t *data, uint32_t len) {
        return this->write(data, (uint32_t) len);
    }

    ~VoiceBotCall() {
        if (buffer) {
            switch_buffer_destroy(&buffer);
        }
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Destroy AiClientCall\n");
    }

    void setBreak() {
        std::lock_guard<std::mutex> lock(stopMutex);
        isBreak = true;
    }

    bool breakStream() {
        std::lock_guard<std::mutex> lock(stopMutex);
        return isBreak;
    }

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;
    std::thread rt;
    switch_buffer_t *buffer = nullptr;
    switch_mutex_t *mutex;
    std::mutex stopMutex;
    bool isBreak = false;

    std::vector<uint8_t> audio_buffer;
    voicebot::AudioRequest request;
    std::string id;
    int32_t model_rate;
    int32_t channel_rate;

    std::unique_ptr<::grpc::ClientReaderWriter<::voicebot::AudioRequest, voicebot::AudioResponse>> rw;
};

class VoiceBotHub {
public:
    explicit VoiceBotHub(const std::shared_ptr<grpc::Channel> &channel_) {
        channel = channel_;
        stub_ = ::voicebot::VoiceBot::NewStub(channel);
    }

    ~VoiceBotHub() {
        stub_.reset();
        channel.reset();
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Destroy AiClient\n");
    }

    bool connected() {
        return channel->GetState(true) == GRPC_CHANNEL_READY;
    }

    VoiceBotCall *Stream(const char *uuid, int32_t model_rate, int32_t channel_rate, std::string &start_message) {
        //todo
        if (!connected()) {
            return nullptr;
        }
        auto *call = new VoiceBotCall(std::string(uuid), model_rate, channel_rate, stub_);
        if (!call->Start(start_message)) {
            delete call;
            return nullptr;
        }
        return call;
    }

private:
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<::voicebot::VoiceBot::Stub> stub_;
};

#endif //MOD_GRPC_VOICE_BOT_CLIENT_H

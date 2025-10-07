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
#include "generated/converse.grpc.pb.h"

class VoiceBotCall {
public:
    explicit VoiceBotCall(std::string callId, int32_t model_rate_, int32_t channel_rate_,
                          std::unique_ptr<::ai_bots::ConverseService::Stub> &stub_) {
        model_rate = model_rate_;
        channel_rate = channel_rate_;
        id = std::move(callId);
        rw = stub_->Converse(&context);
    }

    bool Start(std::string &start_message) {
        ::ai_bots::ConverseRequest req;
        auto config = req.mutable_config();
        config->set_conversation_id(id);
        if (!start_message.empty()) {
            config->set_dialog_id(start_message);
        }

        return rw->Write(req);
    }

    void Listen() {
        rt = std::thread([this] {
            ::ai_bots::ConverseResponse reply;
            switch_audio_resampler_t *resampler = nullptr;
            int out_rate = 24000;
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
                } else if (!reply.variables().empty()) {
                    this->setVars(reply.variables());
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
        size_t target_frame_size = 3200;
        if (model_rate == 8000) {
            target_frame_size = 3200;
        }
        bool ok(true);
        while (audio_buffer.size() >= target_frame_size) {
            std::vector<uint8_t> send_buffer(audio_buffer.begin(), audio_buffer.begin() + target_frame_size);
            ::ai_bots::ConverseRequest req;
            auto input = req.mutable_input();
            input->set_audio_data(send_buffer.data(), send_buffer.size());
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

    void setVars(google::protobuf::Map<std::string, std::string> vars_) {
        std::lock_guard<std::mutex> lock(varsMutex);
        vars = std::move(vars_);
    }

    google::protobuf::Map<std::string, std::string> flushVars() {
        std::lock_guard<std::mutex> lock(varsMutex);
        auto v = std::move(vars);
        vars.clear();
        return v;
    }

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;
    std::thread rt;
    switch_buffer_t *buffer = nullptr;
    switch_mutex_t *mutex;
    std::mutex stopMutex;
    bool isBreak = false;
    google::protobuf::Map<std::string, std::string> vars;
    std::mutex varsMutex;

    std::vector<uint8_t> audio_buffer;
    ai_bots::ConverseRequest request;
    std::string id;
    int32_t model_rate;
    int32_t channel_rate;

    std::unique_ptr<::grpc::ClientReaderWriter<::ai_bots::ConverseRequest, ai_bots::ConverseResponse>> rw;
};

class RecognizeCall {
    public:
    RecognizeCall(std::string dialogId, int32_t _out_rate, int32_t channel_rate_,
                          std::unique_ptr<::ai_bots::ConverseService::Stub> &stub_) {
        out_rate = _out_rate;
        channel_rate = channel_rate_;
        id = std::move(dialogId);
        rw = stub_->Recognize(&context);
    };

    ~RecognizeCall() {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Destroy RecognizeCall\n");
    }

    bool Start() const {
        ::ai_bots::RecognizeRequest req;
        auto config = req.mutable_config();
        config->set_dialog_id(id.c_str());

        return rw->Write(req);
    }

    void setBreak() {
        std::lock_guard<std::mutex> lock(stopMutex);
        isBreak = true;
    }

    bool breakStream() {
        std::lock_guard<std::mutex> lock(stopMutex);
        return isBreak;
    }

    void setVars(google::protobuf::Map<std::string, std::string> vars_) {
        std::lock_guard<std::mutex> lock(varsMutex);
        vars = std::move(vars_);
    }

    google::protobuf::Map<std::string, std::string> flushVars() {
        std::lock_guard<std::mutex> lock(varsMutex);
        auto v = std::move(vars);
        vars.clear();
        return v;
    }

    inline bool write(void *data, uint32_t datalen) {
        ::ai_bots::RecognizeRequest req;
        auto input = req.mutable_input();
        input->set_audio_data(data, datalen);
        return  rw->Write(req);
    }

    inline bool Write(void *data, uint32_t len) {
        return this->write(data, len);
    }

    inline bool Write(uint8_t *data, uint32_t len) {
        return this->write(data, (uint32_t) len);
    }

    bool Listen(int sec) {
        rt = std::thread([this] {
            ::ai_bots::RecognizeResponse reply;

            while (rw->Read(&reply)) {
                if (!ready) {
                    {
                        std::lock_guard<std::mutex> lock(readyMutex);
                        ready = true;
                        readyCondVar.notify_one();
                    }
                }

                if (!reply.variables().empty()) {
                    this->setVars(reply.variables());
                }

                interrupted = reply.interrupted();

                if (reply.is_final()) {
                    break;
                }

            }

            setBreak();
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "close reader\n");

        });

        std::unique_lock<std::mutex> lock(readyMutex);
        readyCondVar.wait_for(lock, std::chrono::seconds(sec), [this] {
            return ready;
        });

        return ready;

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
    int32_t out_rate;
    int32_t channel_rate;
    bool interrupted = false;

private:
    std::unique_ptr<::grpc::ClientReaderWriter<::ai_bots::RecognizeRequest, ai_bots::RecognizeResponse>> rw;
    grpc::ClientContext context;
    std::mutex stopMutex;
    bool isBreak = false;
    std::thread rt;
    ai_bots::ConverseRequest request;
    std::string id;
    google::protobuf::Map<std::string, std::string> vars;
    std::mutex varsMutex;
    std::condition_variable readyCondVar;
    std::mutex readyMutex;
    bool ready = false;
};

class VoiceBotHub {
public:
    explicit VoiceBotHub(const std::shared_ptr<grpc::Channel> &channel_) {
        channel = channel_;
        stub_ = ::ai_bots::ConverseService::NewStub(channel);
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


    RecognizeCall *Recognize(const std::string &dialogId, int32_t out_rate, int32_t channel_rate) {
        //todo
        if (!connected()) {
            return nullptr;
        }
        auto *call = new RecognizeCall(dialogId, out_rate, channel_rate, stub_);
        if (!call->Start()) {
            delete call;
            return nullptr;
        }
        return call;
    }


private:
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<::ai_bots::ConverseService::Stub> stub_;
};

#endif //MOD_GRPC_VOICE_BOT_CLIENT_H

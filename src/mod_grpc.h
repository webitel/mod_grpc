#ifndef MOD_GRPC_LIBRARY_H
#define MOD_GRPC_LIBRARY_H

extern "C" {
#include <switch.h>
#include <switch_curl.h>
#include <sys/select.h>
}

#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "generated/fs.grpc.pb.h"
#include "generated/stream.grpc.pb.h"
#include "Cluster.h"
#include "amd_client.h"
#include "push_client.h"

#define GRPC_SUCCESS_ORIGINATE "grpc_originate_success"

#ifndef MOD_BUILD_VERSION
#define MOD_BUILD_VERSION "DEV"
#endif

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

namespace mod_grpc {

    class Stream {
    public:
        ~Stream();
        switch_core_session_t *session;
        switch_channel_t *channel;
        switch_audio_resampler_t *resampler;
        switch_codec_implementation_t read_impl;
        std::vector<std::string> positive;
        AsyncClientCall* client_;
        switch_vad_t *vad;
        bool stop_vad_on_answer;
        int max_silence_sec;
        int silence_ms;
        int frame_ms;
        bool answered;
    };

    static char *wbt_cache_supported_formats[] = { "wbt_prepare", NULL };

    struct silence_handle {
        int in_cache;
        char *path;
        switch_CURL *curl_handle;
        char curl_error_buff[CURL_ERROR_SIZE];
        long sockfd;
        fd_set fdread;
        size_t samples;
        int silence;
        int forever;
    };

    struct background_pvt {
        int volume_reduction;
        int debug;
        switch_file_handle_t *fh;
        const char *name;
    };

    static switch_status_t wbt_tweaks_on_reporting(switch_core_session_t *session);
    static switch_status_t wbt_tweaks_on_init(switch_core_session_t *session);
    int heartbeat_interval = 0;
    static switch_state_handler_table_t wbt_state_handlers = {
            /*.on_init */ NULL,
            /*.on_routing */ NULL,
            /*.on_execute */ NULL,
            /*.on_hangup */ NULL,
            /*.on_exchange_media */ NULL,
            /*.on_soft_execute */ NULL,
            /*.on_consume_media */ NULL,
            /*.on_hibernate */ NULL,
            /*.on_reset */ NULL,
            /*.on_park */ NULL,
            /*.on_reporting */ wbt_tweaks_on_reporting,
            /*.on_destroy */ NULL
    };

    SWITCH_MODULE_LOAD_FUNCTION(mod_grpc_load);
    SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_grpc_shutdown);

    static inline void fire_event(switch_channel_t *channel, const char *name);

    // Logic and data behind the server's behavior.
    class ApiServiceImpl final : public fs::Api::Service {
    private:
        Status Originate(ServerContext* context, const fs::OriginateRequest* request,
                         fs::OriginateResponse* reply) override;

        Status Execute(ServerContext* context, const fs::ExecuteRequest* request,
                        fs::ExecuteResponse* reply) override;

        Status SetVariables(ServerContext* context, const fs::SetVariablesRequest* request,
                            fs::SetVariablesResponse* reply) override;

        Status Bridge(ServerContext* context, const fs::BridgeRequest* request,
                      fs::BridgeResponse* reply) override;

        Status BridgeCall(ServerContext* context, const fs::BridgeCallRequest* request,
                      fs::BridgeCallResponse* reply) override;

        Status StopPlayback(ServerContext* context, const fs::StopPlaybackRequest* request,
                      fs::StopPlaybackResponse* reply) override;

        Status Hangup(ServerContext* context, const fs::HangupRequest* request,
                        fs::HangupResponse* reply) override;

        Status HangupMatchingVars(ServerContext* context, const fs::HangupMatchingVarsReqeust* request,
                                  fs::HangupMatchingVarsResponse* reply) override;

        Status Queue(ServerContext* context, const fs::QueueRequest* request,
                                  fs::QueueResponse* reply) override;

        Status HangupMany(ServerContext* context, const fs::HangupManyRequest* request,
                      fs::HangupManyResponse* reply) override;

        Status Hold(ServerContext* context, const fs::HoldRequest* request,
                      fs::HoldResponse* reply) override;

        Status UnHold(ServerContext* context, const fs::UnHoldRequest* request,
                      fs::UnHoldResponse* reply) override;

        Status SetProfileVar(ServerContext* context, const fs::SetProfileVarRequest* request,
                      fs::SetProfileVarResponse* reply) override;

        Status ConfirmPush(ServerContext* context, const fs::ConfirmPushRequest* request,
                           fs::ConfirmPushResponse* reply) override;

        Status Broadcast(ServerContext* context, const fs::BroadcastRequest* request,
                           fs::BroadcastResponse* reply) override;

        Status SetEavesdropState(::grpc::ServerContext* context, const ::fs::SetEavesdropStateRequest* request,
                                 ::fs::SetEavesdropStateResponse* reply) override;

        Status BlindTransfer(::grpc::ServerContext* context, const ::fs::BlindTransferRequest* request, ::fs::BlindTransferResponse* response) override;

        Status BreakPark(::grpc::ServerContext* context, const ::fs::BreakParkRequest* request, ::fs::BreakParkResponse* response) override;

    };

    struct Config {
        char const *consul_address;
        int consul_tts_sec;
        int consul_deregister_critical_tts_sec;
        char const *amd_ai_address;
        char const *grpc_host;
        int grpc_port;
        int heartbeat;

        int auto_answer_delay;

        const char *push_service;
        int push_wait_callback;
        int push_fcm_enabled;
        int push_apn_enabled;
    };

    Config loadConfig();

    class ServerImpl final {
    public:
        explicit ServerImpl(Config config_);
        ~ServerImpl() = default;
        void Run();
        void Shutdown();
        std::shared_ptr<grpc::Channel> AMDAiChannel();
        bool AllowAMDAi() const;

        int PushWaitCallback() const;
        int AutoAnswerDelayTime() const;
        bool UseFCM() const;
        bool UseAPN() const;
        AsyncClientCall* AsyncStreamPCMA(int64_t  domain_id, const char *uuid, const char *name, int32_t rate);
        PushClient* GetPushClient();
    private:
        void initServer();
        std::unique_ptr<Server> server_;
        ApiServiceImpl api_;
        Cluster *cluster_;
        std::string server_address_;
        std::thread thread_;
        std::shared_ptr<grpc::Channel> amdAiChannel_;
        bool allowAMDAi;
        grpc::CompletionQueue cq_;

        PushClient *pushClient;
        int push_wait_callback;
        bool push_fcm_enabled;
        bool push_apn_enabled;

        int auto_answer_delay;
        std::unique_ptr<AMDClient> amdClient_;
    };

    ServerImpl *server_;

    extern "C" {
    SWITCH_MODULE_DEFINITION(mod_grpc, mod_grpc_load, mod_grpc_shutdown, nullptr);
    };
}


#endif
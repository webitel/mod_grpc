#ifndef MOD_GRPC_LIBRARY_H
#define MOD_GRPC_LIBRARY_H

extern "C" {
#include <switch.h>
}

#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "generated/fs.grpc.pb.h"
#include "Cluster.h"

#define GRPC_SUCCESS_ORIGINATE "grpc_originate_success"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

namespace mod_grpc {
    static switch_status_t wbt_tweaks_on_reporting(switch_core_session_t *session);
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

    };

    struct PushData {
        std::string call_id;
        std::string from_number;
        std::string from_name;
        std::string direction;
        int auto_answer;
        int delay;
    };

    struct Config {
        char const *consul_address;
        int consul_tts_sec;
        int consul_deregister_critical_tts_sec;
        char const *grpc_host;
        int grpc_port;

        int auto_answer_delay;

        int push_wait_callback;
        int push_fcm_enabled;
        char const *push_fcm_auth;
        char const *push_fcm_uri;

        int push_apn_enabled;
        char const *push_apn_uri;
        char const *push_apn_cert_file;
        char const *push_apn_key_file;
        char const *push_apn_key_pass;
        char const *push_apn_topic;
    };

    Config loadConfig();

    class ServerImpl final {
    public:
        explicit ServerImpl(Config config_);
        ~ServerImpl() = default;
        void Run();
        void Shutdown();

        int PushWaitCallback() const;
        int AutoAnswerDelayTime() const;
        long SendPushFCM(const char *devices, const PushData *data);
        long SendPushAPN(const char *devices, const PushData *data);
        bool UseFCM() const;
        bool UseAPN() const;
    private:
        void initServer();
        std::unique_ptr<Server> server_;
        ApiServiceImpl api_;
        Cluster *cluster_;
        std::string server_address_;
        std::thread thread_;

        int push_wait_callback;
        bool push_fcm_enabled;
        std::string push_fcm_auth;
        std::string push_fcm_uri;

        bool push_apn_enabled;
        std::string push_apn_topic;
        std::string push_apn_uri;
        std::string push_apn_cert_file;
        std::string push_apn_key_file;
        std::string push_apn_key_pass;
        int auto_answer_delay;
    };

    ServerImpl *server_;

    extern "C" {
    SWITCH_MODULE_DEFINITION(mod_grpc, mod_grpc_load, mod_grpc_shutdown, nullptr);
    };
}


#endif
#ifndef MOD_GRPC_LIBRARY_H
#define MOD_GRPC_LIBRARY_H

extern "C" {
#include <switch.h>
}

#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "generated/fs.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;

namespace mod_grpc {
    SWITCH_MODULE_LOAD_FUNCTION(mod_grpc_load);
    SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_grpc_shutdown);

    // Logic and data behind the server's behavior.
    class ApiServiceImpl final : public fs::Api::Service {
        Status Execute(ServerContext* context, const fs::ExecuteRequest* request,
                        fs::ExecuteResponse* reply) override;

        Status Hangup(ServerContext* context, const fs::HangupRequest* request,
                        fs::HangupResponse* reply) override;

        Status Originate(ServerContext* context, const fs::OriginateRequest* request,
                        fs::OriginateResponse* reply) override;

        Status Bridge(ServerContext* context, const fs::BridgeRequest* request,
                        fs::BridgeResponse* reply) override;

    };

    struct Config {
        char const *server_address;
    };

    Config loadConfig();

    class ServerImpl final {
    public:
        explicit ServerImpl(Config config_);
        ~ServerImpl() = default;
        void Run();
        void Shutdown();
    private:
        std::unique_ptr<Server> server_;
        std::string server_address_;
        std::thread thread_;
    };

    ServerImpl *server_;

    extern "C" {
    SWITCH_MODULE_DEFINITION(mod_grpc, mod_grpc_load, mod_grpc_shutdown, nullptr);
    };
}


#endif
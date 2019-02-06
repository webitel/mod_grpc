#include "mod_grpc.h"

#include <iostream>
#include <thread>
#include <sstream>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "generated/fs.grpc.pb.h"

using namespace std;


using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::StatusCode;

namespace mod_grpc {

    Status ApiServiceImpl::Execute(ServerContext *context, const fs::ExecuteRequest *request,
                                   fs::ExecuteResponse *reply)  {
        switch_stream_handle_t stream = { 0 };

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive execute cmd: %s [%s]\n",
                          request->command().c_str(), request->args().c_str());


        if (request->command().length() <= 3) {
            std::string msg("Length of `Command` cannot be less than 3 characters");
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }

        SWITCH_STANDARD_STREAM(stream);

        if (switch_api_execute(request->command().c_str(), request->args().c_str(), nullptr, &stream) == SWITCH_STATUS_FALSE) {
            std::string msg("Command cannot be execute");
            return Status(StatusCode::INTERNAL, msg);
        } else if (stream.data) {
            auto result = std::string((const char*)stream.data);

            if (!result.compare(0, 4, "-ERR")) {
                reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
                reply->mutable_error()->set_message(result.substr(4));

            } else if (!result.compare(0, 6, "-USAGE")) {
                reply->mutable_error()->set_type(fs::ErrorExecute_Type_USAGE);
                reply->mutable_error()->set_message(result.substr(6));
            } else {
                reply->set_data(result);
            }

        } else {
            reply->set_data("todo: empty response");
        }
        switch_safe_free(stream.data);
        return Status::OK;
    }

    Status ApiServiceImpl::Hangup(ServerContext *context, const fs::HangupRequest *request,
                                  fs::HangupResponse *reply) {
        switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive hangup %s [%s]\n",
                          request->uuid().c_str(), request->cause().c_str());

        if (!request->cause().empty()) {
            cause = switch_channel_str2cause(request->cause().c_str());
        }

        if (switch_ivr_kill_uuid(request->uuid().c_str(), cause) != SWITCH_STATUS_SUCCESS) {
            reply->mutable_error()->set_message("No such channel!");
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
        }

        return Status::OK;
    }

    Status ApiServiceImpl::Originate(ServerContext *ctx, const fs::OriginateRequest *request,
                                     fs::OriginateResponse *reply) {
        switch_channel_t *caller_channel;
        switch_core_session_t *caller_session = nullptr;
        uint32_t timeout = 60;
        switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;
        const char *dp, *context;

        if (request->timeout()) {
            timeout = static_cast<uint32_t>(request->timeout());
        }

        if (!request->endpoints_size()) {
            //err
        }

        context = request->context().c_str();
        if (!context) {
            context = "default";
        }

        dp = request->dialplan().c_str();
        if (!dp) {
            dp = "XML";
        }

        std::stringstream aleg;
        for(size_t i = 0; i < request->endpoints().size(); ++i) {
            if(i != 0)
                aleg << ",";
            aleg << request->endpoints()[i];
        }

        if (switch_ivr_originate(nullptr, &caller_session, &cause, aleg.str().c_str(), timeout, nullptr,
                                 request->callername().c_str(), request->callernumber().c_str(), nullptr, nullptr,
                                 SOF_NONE, nullptr) != SWITCH_STATUS_SUCCESS
            || !caller_session) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message(switch_channel_cause2str(cause));
            return Status::OK;
        }

        //TODO add inline dialplan & execute extension
        //caller_channel = switch_core_session_get_channel(caller_session);

        switch_ivr_session_transfer(caller_session, request->destination().c_str(), dp, context);

        reply->set_uuid(switch_core_session_get_uuid(caller_session));
        switch_core_session_rwunlock(caller_session);
        return Status::OK;
    }

    ServerImpl::ServerImpl(Config config_) : server_address_(config_.server_address) { }

    void ServerImpl::Run() {
        thread_ = std::thread([&](){

            ApiServiceImpl api;
            ServerBuilder builder;
            // Listen on the given address without any authentication mechanism.
            builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
            // Register "service" as the instance through which we'll communicate with
            // clients. In this case it corresponds to an *synchronous* service.
            builder.RegisterService(&api);
            // Finally assemble the server.
            server_ = builder.BuildAndStart();
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Server listening on %s\n", server_address_.c_str());
            server_->Wait();
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Server shutdown\n");
        });
    }

    void ServerImpl::Shutdown() {
        if (server_) {
            server_->Shutdown();
        }
        if (thread_.joinable()) {
            thread_.join();
        }
        server_.reset();
    }

    Config loadConfig() {

        auto config  = Config();
        static switch_xml_config_item_t instructions[] = {
                SWITCH_CONFIG_ITEM(
                        "server_address",
                        SWITCH_CONFIG_STRING,
                        CONFIG_RELOADABLE,
                        &config.server_address,
                        "0.0.0.0:50051",
                        nullptr, "server_address", "GRPC server address"),
                SWITCH_CONFIG_ITEM_END()
        };

        if (switch_xml_config_parse_module_settings("grpc.conf", SWITCH_FALSE, instructions) != SWITCH_STATUS_SUCCESS) {
            config.server_address = "0.0.0.0:50051";
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Can't load grpc.conf. Use default GRPC config\n");
        }

        return config;
    }

    SWITCH_MODULE_LOAD_FUNCTION(mod_grpc_load) {
        try {
            *module_interface = switch_loadable_module_create_module_interface(pool, modname);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Module loaded completed\n");
            server_ = new ServerImpl(loadConfig());
            server_->Run();

            return SWITCH_STATUS_SUCCESS;
        } catch (...) { // Exceptions must not propogate to C caller
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error loading GRPC module\n");
            return SWITCH_STATUS_GENERR;
        }

    }

    SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_grpc_shutdown) {
        try {
            server_->Shutdown();
            delete server_;
            google::protobuf::ShutdownProtobufLibrary();
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Module shutting down completed\n");
        } catch (std::exception &ex) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error shutting down GRPC module: %s\n",
                              ex.what());
        } catch (...) { // Exceptions must not propogate to C caller
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
                              "Unknown error shutting down GRPC module\n");
        }
        return SWITCH_STATUS_SUCCESS;
    }

}
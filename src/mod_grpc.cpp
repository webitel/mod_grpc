#include "mod_grpc.h"

#include <iostream>
#include <thread>
#include <sstream>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "generated/fs.grpc.pb.h"

#include "Cluster.h"

#define GRPC_SUCCESS_ORIGINATE "grpc_originate_success"

using namespace std;


using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using grpc::StatusCode;

namespace mod_grpc {

    Status ApiServiceImpl::Originate(ServerContext *ctx, const fs::OriginateRequest *request,
                                     fs::OriginateResponse *reply) {
        switch_channel_t *caller_channel;
        switch_event_t *var_event = NULL;
        switch_core_session_t *caller_session = nullptr;
        uint32_t timeout = 60;
        switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;
        const char *dp, *context;
        const char *separator;

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

        switch (request->strategy()) {
            case fs::OriginateRequest_Strategy_FAILOVER:
                separator = "|";
                break;
            case fs::OriginateRequest_Strategy_MULTIPLE:
                separator = ":_:";
                break;
            default:
                separator = ",";
                break;
        }

        std::stringstream aleg;

        for(size_t i = 0; i < request->endpoints().size(); ++i) {
            if(i != 0)
                aleg << separator;
            aleg << request->endpoints()[i];
        }

        if (request->variables_size() > 0) {
            if (switch_event_create_plain(&var_event, SWITCH_EVENT_CHANNEL_DATA) != SWITCH_STATUS_SUCCESS) {
                std::string msg("Can't create variable event");
                return Status(StatusCode::INTERNAL, msg);
            }

            for (const auto& kv: request->variables()) {
                switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, kv.first.c_str(), kv.second.c_str());
            }
        }

        // todo check version
        if (switch_ivr_originate(nullptr, &caller_session, &cause, aleg.str().c_str(), timeout, nullptr,
                                 request->callername().c_str(), request->callernumber().c_str(), nullptr, var_event,
                                 SOF_NONE, nullptr, nullptr) != SWITCH_STATUS_SUCCESS
            || !caller_session) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message(switch_channel_cause2str(cause));
            reply->set_error_code(static_cast<::google::protobuf::int32>(cause));

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Originate error %s\n", switch_channel_cause2str(cause));
            goto done;
        }

        caller_channel = switch_core_session_get_channel(caller_session);
        switch_channel_set_variable(caller_channel, GRPC_SUCCESS_ORIGINATE, "true");

        if (!request->extensions().empty()) {
            switch_caller_extension_t *extension = nullptr;
            if ((extension = switch_caller_extension_new(caller_session,
                                                         request->callername().c_str(),
                                                         request->callernumber().c_str())) == 0) {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Memory Error!\n");
                abort();
            }

            for (const auto &ex : request->extensions()) {
                switch_caller_extension_add_application(caller_session, extension,
                                                        ex.appname().c_str(),
                                                        ex.args().c_str());
            }

            switch_channel_set_caller_extension(caller_channel, extension);
            switch_channel_set_state(caller_channel, CS_EXECUTE);
        } else {
            switch_ivr_session_transfer(caller_session, request->destination().c_str(), dp, context);
        }

        reply->set_uuid(switch_core_session_get_uuid(caller_session));

        switch_core_session_rwunlock(caller_session);

        done:
        if (var_event) {
            switch_event_destroy(&var_event);
        }
        return Status::OK;
    }

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

    Status ApiServiceImpl::SetVariables(ServerContext *context, const fs::SetVariablesRequest *request,
                                        fs::SetVariablesResponse *reply) {

        switch_core_session_t *psession = nullptr;

        if (request->uuid().empty() || request->variables().empty()) {
            std::string msg("uuid or variables is required");
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }

        if ((psession = switch_core_session_locate(request->uuid().c_str()))) {
            switch_channel_t *channel = switch_core_session_get_channel(psession);

            for (const auto& kv: request->variables()) {
                switch_channel_set_variable(channel, kv.first.c_str(), kv.second.c_str());
            }
            switch_core_session_rwunlock(psession);
        } else {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("no such channel");
        }

        return Status::OK;

    }

    Status ApiServiceImpl::Bridge(ServerContext *context, const fs::BridgeRequest *request,
                                  fs::BridgeResponse *reply) {
        switch_status_t status;
        if (request->leg_a_id().empty() || request->leg_b_id().empty()) {
            std::string msg("leg_a_id or leg_b_id is required");
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive bridge request %s & %s\n",
                          request->leg_a_id().c_str(), request->leg_b_id().c_str());

        if ((status = switch_ivr_uuid_bridge(request->leg_a_id().c_str(), request->leg_b_id().c_str())) != SWITCH_STATUS_SUCCESS) {
            if (!request->leg_b_reserve_id().empty()) {
                if ((status = switch_ivr_uuid_bridge(request->leg_a_id().c_str(), request->leg_b_reserve_id().c_str())) == SWITCH_STATUS_SUCCESS) {
                    reply->set_uuid(request->leg_b_reserve_id());
                }
            }
        } else {
            reply->set_uuid(request->leg_b_id());
        }

        if (status != SWITCH_STATUS_SUCCESS) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("Invalid id");
        }

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

        // FIXME add new function
        if (request->reporting()) {
            switch_core_session_t *psession = nullptr;
            if ((psession = switch_core_session_locate(request->uuid().c_str()))) {
                switch_channel_t *channel = switch_core_session_get_channel(psession);

                switch_channel_set_variable(channel, "cc_reporting_at", std::to_string(unixTimestamp()).c_str());

                switch_core_session_rwunlock(psession);
            }
        }

        if (switch_ivr_kill_uuid(request->uuid().c_str(), cause) != SWITCH_STATUS_SUCCESS) {
            reply->mutable_error()->set_message("No such channel!");
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
        }

        return Status::OK;
    }

    Status ApiServiceImpl::HangupMatchingVars(ServerContext *context, const fs::HangupMatchingVarsReqeust *request,
                                              fs::HangupMatchingVarsResponse *reply) {
        switch_call_cause_t cause = SWITCH_CAUSE_MANAGER_REQUEST;
        switch_event_t *vars = nullptr;
        uint32_t count = 0;

        if (request->variables().empty()) {
            std::string msg("variables is required");
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }

        if (!request->cause().empty()) {
            cause = switch_channel_str2cause(request->cause().c_str());
        }

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive hangup matching variables request\n");

        switch_event_create(&vars, SWITCH_EVENT_CLONE);

        for (const auto& kv: request->variables()) {
            switch_event_add_header_string(vars, SWITCH_STACK_BOTTOM, kv.first.c_str(), kv.second.c_str());
        }

        count = switch_core_session_hupall_matching_vars_ans(vars, cause, static_cast<switch_hup_type_t>(SHT_UNANSWERED | SHT_ANSWERED));
        reply->set_count(count);

        if (vars) {
            switch_event_destroy(&vars);
        }
        return Status::OK;
    }

    Status ApiServiceImpl::Queue(ServerContext *context, const fs::QueueRequest *request, fs::QueueResponse *reply) {
        switch_core_session_t *psession = nullptr;
        if ((psession = switch_core_session_locate(request->id().c_str()))) {
            switch_media_flag_t flags = SMF_ECHO_ALEG;
            switch_channel_t *channel = switch_core_session_get_channel(psession);

            if (!request->variables().empty()) {
                for (const auto &kv: request->variables()) {
                    switch_channel_set_variable(channel, kv.first.c_str(), kv.second.c_str());
                }
            }

            if (!request->playback_file().empty()) {
                flags |= SMF_LOOP; // FIXME add parameter
                switch_ivr_broadcast(request->id().c_str(), request->playback_file().c_str(),  flags);
            }

            switch_core_session_rwunlock(psession);

        } else {
            reply->mutable_error()->set_message("No such channel!");
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
        }

        return Status::OK;
    }

    ServerImpl::ServerImpl(Config config_) {
        if (!config_.grpc_host) {
            char ipV4_[80];
            switch_find_local_ip(ipV4_, sizeof(ipV4_), nullptr, AF_INET);
            config_.grpc_host = std::string(ipV4_).c_str();
        }
        server_address_ = std::string(config_.grpc_host)  + ":" + std::to_string(config_.grpc_port);

        if (config_.consul_address) {
            cluster_ = new Cluster(config_.consul_address, config_.grpc_host, config_.grpc_port,
                                   config_.consul_tts_sec, config_.consul_deregister_critical_tts_sec);
        }
    }

    void ServerImpl::Run() {
        initServer();
        thread_ = std::thread([&](){
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

        delete cluster_;
        server_.reset();
    }

    void ServerImpl::initServer() {
        ServerBuilder builder;
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
        // Register "service" as the instance through which we'll communicate with
        // clients. In this case it corresponds to an *synchronous* service.
        builder.RegisterService(&api_);
        // Finally assemble the server.
        server_ = builder.BuildAndStart();

        if (!server_) {
            this->Shutdown();
            throw "Can't bind to address: " + server_address_;
        }
    }

    Config loadConfig() {

        auto config  = Config();
        static switch_xml_config_item_t instructions[] = {
                SWITCH_CONFIG_ITEM(
                        "grpc_host",
                        SWITCH_CONFIG_STRING,
                        CONFIG_RELOADABLE,
                        &config.grpc_host,
                        nullptr,
                        nullptr, "grpc_host", "GRPC server address"),
                SWITCH_CONFIG_ITEM(
                        "consul_ttl_sec",
                        SWITCH_CONFIG_INT,
                        CONFIG_RELOADABLE,
                        &config.consul_tts_sec,
                        (void *) 60,
                        nullptr, nullptr, "GRPC TTL"),
                SWITCH_CONFIG_ITEM(
                        "consul_deregister_critical_ttl_sec",
                        SWITCH_CONFIG_INT,
                        CONFIG_RELOADABLE,
                        &config.consul_deregister_critical_tts_sec,
                        (void *) 120,
                        nullptr, nullptr, "GRPC degeregister critical TTL"),
                SWITCH_CONFIG_ITEM(
                        "grpc_port",
                        SWITCH_CONFIG_INT,
                        CONFIG_RELOADABLE,
                        &config.grpc_port,
                        (void *) 50051,
                        nullptr, nullptr, "GRPC server port"),
                SWITCH_CONFIG_ITEM(
                        "consul_address",
                        SWITCH_CONFIG_STRING,
                        CONFIG_RELOADABLE,
                        &config.consul_address,
                        nullptr,
                        nullptr, "consul_address", "Consul address"),
                SWITCH_CONFIG_ITEM_END()
        };

        if (switch_xml_config_parse_module_settings("grpc.conf", SWITCH_FALSE, instructions) != SWITCH_STATUS_SUCCESS) {
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
            google::protobuf::ShutdownProtobufLibrary(); //FIXME CRASH ???
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
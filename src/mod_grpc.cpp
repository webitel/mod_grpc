#include "mod_grpc.h"

#include <iostream>
#include <thread>
#include <sstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <numeric>
#include "generated/fs.grpc.pb.h"
#include "generated/stream.grpc.pb.h"

#include "Cluster.h"
#include "switch_core.h"

#define BUG_STREAM_NAME "wbt_amd"
#define MODEL_RATE 8000
#define AMD_EVENT_NAME "amd::info"
#define AMD_EXECUTE_VARIABLE "amd_on_positive"

using namespace std;


using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ClientAsyncResponseReader;
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

        for (int i = 0; i < request->endpoints().size(); ++i) {
            if (i != 0)
                aleg << separator;
            aleg << request->endpoints()[i];
        }

        if (switch_event_create_plain(&var_event, SWITCH_EVENT_CHANNEL_DATA) != SWITCH_STATUS_SUCCESS) {
            std::string msg("Can't create variable event");
            return Status(StatusCode::INTERNAL, msg);
        }
        switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, "wbt_originate", "true");
        if (request->variables_size() > 0) {
            for (const auto &kv: request->variables()) {
                switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, kv.first.c_str(), kv.second.c_str());
            }
        }

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "originate: %s\n", aleg.str().c_str());

        // todo check version
        if (switch_ivr_originate(nullptr, &caller_session, &cause, aleg.str().c_str(), timeout, nullptr,
                                 request->callername().c_str(), request->callernumber().c_str(), nullptr, var_event,
                                 SOF_NONE, nullptr, nullptr) != SWITCH_STATUS_SUCCESS
            || !caller_session) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message(switch_channel_cause2str(cause));
            reply->set_error_code(static_cast<::google::protobuf::int32>(cause));

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Originate error %s\n",
                              switch_channel_cause2str(cause));
            goto done;
        }

        caller_channel = switch_core_session_get_channel(caller_session);
        switch_channel_set_variable(caller_channel, GRPC_SUCCESS_ORIGINATE, "true");

        if (!request->extensions().empty()) {
            switch_caller_extension_t *extension = nullptr;
            switch_core_session_reset(caller_session, SWITCH_TRUE, SWITCH_TRUE);
            switch_channel_clear_flag(caller_channel, CF_ORIGINATING);

            /* clear all state handlers */
            switch_channel_clear_state_handler(caller_channel, NULL);

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
            switch_channel_set_state(caller_channel, CS_RESET);
            switch_channel_wait_for_state(caller_channel, nullptr, CS_RESET);
            switch_channel_set_state(caller_channel, CS_EXECUTE);
        } else {
            switch_ivr_session_transfer(caller_session, request->destination().c_str(), dp, context);
        }

        reply->set_uuid(switch_core_session_get_uuid(caller_session));
        if (caller_session && !request->check_id().empty()) {
            switch_core_session_t *check_session = switch_core_session_locate(request->check_id().c_str());
            bool hangup = !check_session;
            if (check_session) {
                switch_channel_t *check_channel = switch_core_session_get_channel(check_session);
                hangup = !switch_channel_ready(check_channel);
                switch_core_session_rwunlock(check_session);
            }

            if (hangup) {
                switch_channel_hangup(caller_channel, SWITCH_CAUSE_ORIGINATOR_CANCEL);
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Originate error, not found check id %s\n",
                                  request->check_id().c_str());
            }
        }

        switch_core_session_rwunlock(caller_session);

        done:
        if (var_event) {
            switch_event_destroy(&var_event);
        }
        return Status::OK;
    }

    Status ApiServiceImpl::Execute(ServerContext *context, const fs::ExecuteRequest *request,
                                   fs::ExecuteResponse *reply) {
        switch_stream_handle_t stream = {0};

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive execute cmd: %s [%s]\n",
                          request->command().c_str(), request->args().c_str());


        if (request->command().length() <= 3) {
            std::string msg("Length of `Command` cannot be less than 3 characters");
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }

        SWITCH_STANDARD_STREAM(stream);

        if (switch_api_execute(request->command().c_str(), request->args().c_str(), nullptr, &stream) ==
            SWITCH_STATUS_FALSE) {
            std::string msg("Command cannot be execute");
            return Status(StatusCode::INTERNAL, msg);
        } else if (stream.data) {
            auto result = std::string((const char *) stream.data);

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

            for (const auto &kv: request->variables()) {
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
        int bridged = 1;

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "receive bridge %s to %s\n",
                          request->leg_a_id().c_str(), request->leg_b_id().c_str());

        switch_core_session_t *session;
        session = switch_core_session_locate(request->leg_b_id().c_str());
        if (session) {
            switch_channel_t *channel = switch_core_session_get_channel(session);
            int cnt = 0;
            while (switch_channel_ready(channel) && cnt < 10 && (switch_channel_test_flag(channel, CF_ORIGINATING)) && switch_channel_up(channel) ) {
                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "waiting for done originate\n");
                switch_ivr_sleep(session, 30, SWITCH_TRUE, NULL);
                cnt++;
            }

            switch_core_session_rwunlock(session);
        }

        if (switch_ivr_uuid_bridge(request->leg_a_id().c_str(), request->leg_b_id().c_str()) != SWITCH_STATUS_SUCCESS) {
            bridged = 0;
        }

        if (bridged) {
            switch_core_session_t *session;
            session = switch_core_session_locate(request->leg_b_id().c_str());
            if (session) {
                switch_channel_t *channel = switch_core_session_get_channel(session);
                if (switch_channel_ready(channel)) {
                    switch_channel_wait_for_flag(channel, CF_BRIDGED, SWITCH_TRUE, 1500, nullptr);
                }

                switch_core_session_rwunlock(session);
            } else {
                bridged = 0;
            }
        }

        if (!bridged) {
            reply->mutable_error()->set_message("not found call id");
        }

        return Status::OK;
    }

    Status ApiServiceImpl::Hangup(ServerContext *context, const fs::HangupRequest *request,
                                  fs::HangupResponse *reply) {
        switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;
        switch_core_session_t *session;

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive hangup %s [%s]\n",
                          request->uuid().c_str(), request->cause().c_str());

        if (!request->cause().empty()) {
            cause = switch_channel_str2cause(request->cause().c_str());
        }

        if (zstr(request->uuid().c_str()) || !(session = switch_core_session_locate(request->uuid().c_str()))) {
            reply->mutable_error()->set_message("No such channel!");
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
        } else {
            switch_channel_t *channel = switch_core_session_get_channel(session);
            for (const auto &kv: request->variables()) {
                switch_channel_set_variable(channel, kv.first.c_str(), kv.second.c_str());

                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Set hangup var %s [%s = %s]\n",
                                  request->uuid().c_str(), kv.first.c_str(), kv.second.c_str());
            }

            if (request->reporting()) {
                switch_channel_set_variable(channel, "cc_reporting_at", std::to_string(unixTimestamp()).c_str());
            }

            switch_channel_set_variable(channel, "grpc_send_hangup", "1");

            switch_core_session_rwunlock(session);
            switch_channel_hangup(channel, cause);
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

        for (const auto &kv: request->variables()) {
            switch_event_add_header_string(vars, SWITCH_STACK_BOTTOM, kv.first.c_str(), kv.second.c_str());
        }

        count = switch_core_session_hupall_matching_vars_ans(vars, cause,
                                                             static_cast<switch_hup_type_t>(SHT_UNANSWERED |
                                                                                            SHT_ANSWERED));
        reply->set_count(count);

        if (vars) {
            switch_event_destroy(&vars);
        }
        return Status::OK;
    }

    Status ApiServiceImpl::Queue(ServerContext *context, const fs::QueueRequest *request, fs::QueueResponse *reply) {
        switch_core_session_t *psession = nullptr;
        if ((psession = switch_core_session_force_locate(request->id().c_str()))) {
            switch_media_flag_t flags = SMF_ECHO_ALEG;
            switch_channel_t *channel = switch_core_session_get_channel(psession);

            if (!switch_channel_up(channel)) {
                switch_core_session_rwunlock(psession);
                return Status::CANCELLED;
            }

            if (!request->playback_file().empty()) {
                flags |= SMF_PRIORITY; // FIXME add parameter
                if (switch_ivr_broadcast(request->id().c_str(), request->playback_file().c_str(), flags) !=
                    SWITCH_STATUS_SUCCESS) {
                    switch_core_session_rwunlock(psession);
                    return Status::CANCELLED;
                }
            }

            if (!request->variables().empty()) {
                for (const auto &kv: request->variables()) {
                    switch_channel_set_variable_var_check(channel, kv.first.c_str(), kv.second.c_str(), SWITCH_FALSE);
                }
            }

            switch_core_session_rwunlock(psession);

        } else {
            reply->mutable_error()->set_message("No such channel!");
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            return Status::CANCELLED;
        }

        return Status::OK;
    }

    Status ApiServiceImpl::HangupMany(ServerContext *context, const fs::HangupManyRequest *request,
                                      fs::HangupManyResponse *reply) {
        switch_call_cause_t cause = SWITCH_CAUSE_NORMAL_CLEARING;

        if (!request->cause().empty()) {
            cause = switch_channel_str2cause(request->cause().c_str());
        }

        for (const auto &id: request->id()) {
            switch_core_session_t *session;

            if (!id.empty() && (session = switch_core_session_locate(id.c_str()))) {
                switch_channel_t *channel = switch_core_session_get_channel(session);
                switch_channel_set_variable(channel, "grpc_send_hangup", "1");

                switch_channel_hangup(channel, cause);
                switch_core_session_rwunlock(session);

                reply->add_id(id);
            }
        }

        return Status::OK;
    }

    Status ApiServiceImpl::Hold(ServerContext *context, const fs::HoldRequest *request, fs::HoldResponse *reply) {
        for (const auto &id: request->id()) {
            switch_core_session_t *session;

            if (!id.empty() && (session = switch_core_session_locate(id.c_str()))) {
                if (!switch_channel_test_flag(switch_core_session_get_channel(session), CF_PROTO_HOLD)) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive Hold request %s\n", id.c_str());
                    switch_core_media_toggle_hold(session, 1);
                    reply->add_id(id);
                }

                switch_core_session_rwunlock(session);
            }
        }

        return Status::OK;
    }

    Status ApiServiceImpl::UnHold(ServerContext *context, const fs::UnHoldRequest *request, fs::UnHoldResponse *reply) {
        for (const auto &id: request->id()) {
            switch_core_session_t *session;

            if (!id.empty() && (session = switch_core_session_locate(id.c_str()))) {
                if (switch_channel_test_flag(switch_core_session_get_channel(session), CF_PROTO_HOLD)) {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive UnHold request %s\n", id.c_str());
                    switch_core_media_toggle_hold(session, 0);
                    reply->add_id(id);
                }

                switch_core_session_rwunlock(session);
            }
        }

        return Status::OK;
    }

    Status ApiServiceImpl::BridgeCall(ServerContext *context, const fs::BridgeCallRequest *request,
                                      fs::BridgeCallResponse *reply) {
        switch_status_t status = SWITCH_STATUS_TERM;
        switch_core_session_t *leg_a_s = nullptr, *leg_b_s = nullptr;
        switch_channel_t *chan_a_s = nullptr, *chan_b_s = nullptr;

        const char *cc_to_agent_id = nullptr;
        const char *cc_from_attempt_id = nullptr;
        const char *cc_to_attempt_id = nullptr;
        const char *pa = nullptr;
        const char *pb = nullptr;

        if (request->leg_a_id().empty() || request->leg_b_id().empty()) {
            std::string msg("leg_a_id or leg_b_id is required");
            return Status(StatusCode::INVALID_ARGUMENT, msg);
        }

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive bridgeCall request %s & %s\n",
                          request->leg_a_id().c_str(), request->leg_b_id().c_str());

        leg_a_s = switch_core_session_locate(request->leg_a_id().c_str());
        leg_b_s = switch_core_session_locate(request->leg_b_id().c_str());

        if (leg_a_s) {
            chan_a_s = switch_core_session_get_channel(leg_a_s);
        }

        if (leg_b_s) {
            chan_b_s = switch_core_session_get_channel(leg_b_s);
        }

        if (chan_a_s) {
            pa = switch_channel_get_partner_uuid(chan_a_s);
        }

        if (chan_b_s) {
            pb = switch_channel_get_partner_uuid(chan_b_s);
        }

        if (leg_a_s && leg_b_s && chan_a_s && chan_b_s && pa && pb
            && switch_channel_test_flag(chan_a_s, CF_BRIDGED) && switch_channel_test_flag(chan_b_s, CF_BRIDGED)) {

            if (switch_channel_get_partner_uuid(chan_a_s)) {
                switch_channel_set_variable_partner(chan_a_s, "wbt_transfer_from", request->leg_b_id().c_str());

                cc_to_agent_id = switch_channel_get_variable_dup(chan_a_s, "cc_agent_id", SWITCH_FALSE, -1);
                cc_from_attempt_id = switch_channel_get_variable_dup(chan_a_s, "cc_attempt_id", SWITCH_FALSE, -1);
            }

            if (cc_to_agent_id) {
                switch_channel_set_variable_partner(chan_b_s, "wbt_transfer_to_agent", cc_to_agent_id);
            }

            if (cc_from_attempt_id) {
                switch_channel_set_variable_partner(chan_b_s, "wbt_transfer_from_attempt", cc_from_attempt_id);
            }

            switch_channel_set_variable_partner(chan_b_s, "wbt_transfer_to", request->leg_a_id().c_str());

            if (cc_from_attempt_id &&
                (cc_to_attempt_id = switch_channel_get_variable_partner(chan_b_s, "cc_attempt_id"))) {
                switch_channel_set_variable_partner(chan_a_s, "wbt_transfer_to_attempt",
                                                    std::string(cc_to_attempt_id).c_str());
            }

            if (!switch_channel_down(chan_a_s) && !switch_core_media_bug_count(leg_a_s, "session_record") &&
                    switch_true(switch_channel_get_variable_partner(chan_a_s, "recording_follow_transfer")) &&
                    !switch_core_media_bug_count(leg_b_s, "session_record")) {

                auto pas = switch_core_session_locate(pa);
                if (pas) {
                    switch_ivr_transfer_recordings(pas, leg_a_s);
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Transfer record from (%s) to (%s)\n",
                                      switch_core_session_get_name(pas), switch_core_session_get_name(leg_a_s));
                    switch_core_session_rwunlock(pas);
                }
            }

            if (!request->variables().empty()) {
                for (const auto &kv: request->variables()) {
                    switch_channel_set_variable_var_check(chan_b_s, kv.first.c_str(), kv.second.c_str(), SWITCH_FALSE);
                    switch_channel_set_variable_var_check(chan_a_s, kv.first.c_str(), kv.second.c_str(), SWITCH_FALSE);
                }
            }

            if ((status = switch_ivr_uuid_bridge(request->leg_a_id().c_str(), request->leg_b_id().c_str())) !=
                SWITCH_STATUS_SUCCESS) {
                // todo clean variables - transfer fail && rollback transfer recordings
            } else {
                reply->set_uuid(request->leg_b_id());
            }
        }

        if (leg_a_s) {
            switch_core_session_rwunlock(leg_a_s);
        }

        if (leg_b_s) {
            switch_core_session_rwunlock(leg_b_s);
        }

        if (status != SWITCH_STATUS_SUCCESS) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("Invalid id");
        }

        return Status::OK;
    }

    Status ApiServiceImpl::StopPlayback(ServerContext *context, const fs::StopPlaybackRequest *request,
                                        fs::StopPlaybackResponse *reply) {
        switch_core_session_t *session;

        if (!request->id().empty() && (session = switch_core_session_locate(request->id().c_str()))) {
            switch_channel_t *channel = switch_core_session_get_channel(session);
//            switch_channel_clear_flag(channel, CF_HOLD);
            if (switch_channel_test_flag(channel, CF_BROADCAST)) {
                switch_channel_stop_broadcast(channel);
                switch_channel_wait_for_flag(channel, CF_BROADCAST, SWITCH_FALSE, 5000, NULL);
            }

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "executed stop_playback\n");
            switch_core_session_rwunlock(session);
        }

        return Status::OK;
    }

    Status ApiServiceImpl::SetProfileVar(ServerContext *context, const fs::SetProfileVarRequest *request,
                                         fs::SetProfileVarResponse *reply) {
        switch_core_session_t *session;
        if (!request->id().empty() && (session = switch_core_session_locate(request->id().c_str()))) {
            switch_channel_t *channel = switch_core_session_get_channel(session);
            if (request->variables_size() > 0) {
                for (const auto &kv: request->variables()) {
                    switch_channel_set_profile_var(channel, kv.first.c_str(), kv.second.c_str());
                }
            }
            switch_core_session_rwunlock(session);
        } else {
            return Status::CANCELLED;
        }

        return Status::OK;
    }

    Status ApiServiceImpl::ConfirmPush(ServerContext *context, const fs::ConfirmPushRequest *request,
                                       fs::ConfirmPushResponse *reply) {

        if (request->id().empty()) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("bad request: no call id");
            return Status::OK;
        }

        switch_core_session_t *session;
        session = switch_core_session_locate(request->id().c_str());
        if (session) {
            switch_channel_t *channel = switch_core_session_get_channel(session);
            //todo fixme CF_SLA_INTERCEPT -> CF_DEVICES_CHANGED
            switch_channel_set_flag(channel, CF_SLA_INTERCEPT);
            switch_core_session_rwunlock(session);
        } else {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("No such channel!");
        }

        return Status::OK;
    }

    Status ApiServiceImpl::Broadcast(ServerContext *context, const fs::BroadcastRequest *request,
                                     fs::BroadcastResponse *reply) {
        if (request->id().empty()) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("bad request: no call id");
            return Status::OK;
        }

        switch_core_session_t *session;
        session = switch_core_session_locate(request->id().c_str());
        if (session) {
            switch_media_flag_t flags = SMF_NONE;
            switch_channel_t *channel = switch_core_session_get_channel(session);
            auto leg = request->leg();
            if (leg == "both") {
                flags |= (SMF_ECHO_ALEG | SMF_ECHO_BLEG);
            } else if (leg == "aleg") {
                flags |= SMF_ECHO_ALEG;
            } else if (leg == "bleg") {
                flags &= ~SMF_HOLD_BLEG;
                flags |= SMF_ECHO_BLEG;
            } else if (leg == "holdb") {
                flags &= ~SMF_ECHO_BLEG;
                flags |= SMF_HOLD_BLEG;
            } else {
                flags = SMF_ECHO_ALEG | SMF_HOLD_BLEG;
            }

            if (request->wait_for_answer()) {
                while (!switch_channel_test_flag(channel, CF_PARK) && switch_channel_ready(channel)) {
                    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "wait for answer\n");
                    switch_ivr_sleep(session, 50, SWITCH_TRUE, NULL);
                }
            }
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "broadcast: %s\n", request->args().c_str());
            switch_core_session_rwunlock(session);

            if (switch_ivr_broadcast(request->id().c_str(), request->args().c_str(), flags) == SWITCH_STATUS_SUCCESS) {
                *reply->mutable_data() = "OK";
            } else {
                reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
                reply->mutable_error()->set_message("invalid uuid!");
            }

        } else {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("No such channel!");
        }

        return Status::OK;
    }

    Status ApiServiceImpl::SetEavesdropState(::grpc::ServerContext* context, const ::fs::SetEavesdropStateRequest* request,
                                             ::fs::SetEavesdropStateResponse* reply) {

        if (request->id().empty()) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("bad request: no call id");
            return Status::OK;
        }

        switch_core_session_t *session;
        session = switch_core_session_locate(request->id().c_str());
        if (session) {
            switch_channel_t *channel = switch_core_session_get_channel(session);
            std::string dtmf;
            if (request->state() == "muted") {
                dtmf = "0";
                switch_channel_set_variable(channel, WBT_EAVESDROP_STATE, request->state().c_str());
            } else if (request->state() == "conference") {
                dtmf = "3";
                switch_channel_set_variable(channel, WBT_EAVESDROP_STATE, request->state().c_str());
            } else {
                dtmf = "2";
                switch_channel_set_variable(channel, WBT_EAVESDROP_STATE, "prompt");
            }

            if (switch_channel_queue_dtmf_string(switch_core_session_get_channel(session), dtmf.c_str()) == SWITCH_STATUS_GENERR) {

            }
            switch_core_session_rwunlock(session);

            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "eavesdrop: %s set %s\n",
                              request->id().c_str(), dtmf.c_str());

            fire_event(channel, EAVESDROP_EVENT_NAME);
        } else {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("No such channel!");
        }

        return Status::OK;
    }

    Status ApiServiceImpl::BlindTransfer(::grpc::ServerContext *context, const ::fs::BlindTransferRequest *request,
                                         ::fs::BlindTransferResponse *reply) {

        if (request->id().empty()) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("bad request: id is required");
            return Status::OK;
        }


        if (request->destination().empty()) {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("bad request: destination is required");
            return Status::OK;
        }

        switch_core_session_t *session;
        session = switch_core_session_locate(request->id().c_str());
        if (session) {
            switch_channel_t *channel = switch_core_session_get_channel(session);

            for (const auto &kv: request->variables()) {
                switch_channel_set_variable_var_check(channel, kv.first.c_str(), kv.second.c_str(), SWITCH_FALSE);
            }

            if (switch_ivr_session_transfer(session, request->destination().c_str(), request->dialplan().c_str(), request->context().c_str()) != SWITCH_STATUS_SUCCESS) {
                reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
                reply->mutable_error()->set_message("internal error");
            }

            switch_core_session_rwunlock(session);
        } else {
            reply->mutable_error()->set_type(fs::ErrorExecute_Type_ERROR);
            reply->mutable_error()->set_message("No such channel!");
        }

        return Status::OK;
    }

    Status ApiServiceImpl::BreakPark(::grpc::ServerContext *context, const ::fs::BreakParkRequest *request,
                                     ::fs::BreakParkResponse *response) {
        response->set_ok(false);

        if (!request->id().empty()) {
            switch_core_session_t *session;
            session = switch_core_session_locate(request->id().c_str());
            if (session) {
                switch_channel_t *channel = switch_core_session_get_channel(session);

                if (switch_channel_test_flag(channel, CF_PARK)) {
                    switch_channel_clear_flag(channel, CF_PARK);
                    response->set_ok(true);
                }

                switch_core_session_rwunlock(session);
                return Status::OK;
            }
        }

        return Status::OK;
    }

    ServerImpl::ServerImpl(Config config_) {
        if (!config_.grpc_host) {
            char ipV4_[80];
            switch_find_local_ip(ipV4_, sizeof(ipV4_), nullptr, AF_INET);
            config_.grpc_host = std::string(ipV4_).c_str();
        }
        server_address_ = std::string(config_.grpc_host) + ":" + std::to_string(config_.grpc_port);

        if (config_.consul_address) {
            cluster_ = new Cluster(config_.consul_address, config_.grpc_host, config_.grpc_port,
                                   config_.consul_tts_sec, config_.consul_deregister_critical_tts_sec);
        }

        if (config_.push_service) {
            this->pushClient = new PushClient(std::string(config_.push_service));
        } else {
            this->pushClient = nullptr;
        }
        this->push_wait_callback = config_.push_wait_callback;
        this->push_fcm_enabled = config_.push_fcm_enabled && this->pushClient;
        this->push_apn_enabled = config_.push_apn_enabled && this->pushClient;

        this->auto_answer_delay = config_.auto_answer_delay;
        this->allowAMDAi = false;
        if (config_.amd_ai_address) {
            auto amd_ai_address = std::string(config_.amd_ai_address);
            if (!amd_ai_address.empty() && amd_ai_address.size() > 5) {
                this->amdAiChannel_ = grpc::CreateChannel(amd_ai_address, grpc::InsecureChannelCredentials());
                this->allowAMDAi = true;
                this->amdClient_.reset(new AMDClient(this->amdAiChannel_));
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Connect to AMD AI %s\n",
                                  amd_ai_address.c_str());
            }
        }
    }

    std::shared_ptr<grpc::Channel> ServerImpl::AMDAiChannel() {
        return amdAiChannel_;
    }

    bool ServerImpl::AllowAMDAi() const {
        return allowAMDAi;
    }

    void ServerImpl::Run() {
        initServer();
        thread_ = std::thread([&]() {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Server listening on %s\n",
                              server_address_.c_str());
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

        if (amdAiChannel_ && amdAiChannel_->GetState(false) != GRPC_CHANNEL_SHUTDOWN) {
            amdAiChannel_.reset();
        }

        if (amdClient_) {
            amdClient_.reset();
        }

        if (pushClient) {
            delete pushClient;
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

        auto config = Config();
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
                SWITCH_CONFIG_ITEM(
                        "amd_ai_address",
                        SWITCH_CONFIG_STRING,
                        CONFIG_RELOADABLE,
                        &config.amd_ai_address,
                        nullptr,
                        nullptr, "amd_ai_address", "AMD stream AI address"),
                SWITCH_CONFIG_ITEM(
                        "auto_answer_delay",
                        SWITCH_CONFIG_INT,
                        CONFIG_RELOADABLE,
                        &config.auto_answer_delay,
                        (void *) 0,
                        nullptr, nullptr, "Auto answer delay time is seconds"),
                SWITCH_CONFIG_ITEM(
                        "push_wait_callback",
                        SWITCH_CONFIG_INT,
                        CONFIG_RELOADABLE,
                        &config.push_wait_callback,
                        2000,
                        nullptr, "push_wait_callback", "Push wait callback time"),
                SWITCH_CONFIG_ITEM(
                        "push_service",
                        SWITCH_CONFIG_STRING,
                        CONFIG_RELOADABLE,
                        &config.push_service,
                        nullptr,
                        nullptr, "push_service", "Push service endpoint"),
                SWITCH_CONFIG_ITEM(
                        "heartbeat",
                        SWITCH_CONFIG_INT,
                        CONFIG_RELOADABLE,
                        &config.heartbeat,
                        0,
                        nullptr, "heartbeat", "Enable Media Heartbeat"),
                SWITCH_CONFIG_ITEM(
                        "push_fcm_enabled",
                        SWITCH_CONFIG_BOOL,
                        CONFIG_RELOADABLE,
                        &config.push_fcm_enabled,
                        0,
                        nullptr, "push_fcm_enabled", "Enable FCM"),
                SWITCH_CONFIG_ITEM(
                        "push_apn_enabled",
                        SWITCH_CONFIG_BOOL,
                        CONFIG_RELOADABLE,
                        &config.push_apn_enabled,
                        0,
                        nullptr, "push_apn_enabled", "Enable APN"),
                SWITCH_CONFIG_ITEM_END()
        };

        if (switch_xml_config_parse_module_settings("grpc.conf", SWITCH_FALSE, instructions) != SWITCH_STATUS_SUCCESS) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                              "Can't load grpc.conf. Use default GRPC config\n");
        }

        return config;
    }

    std::string toJson(const PushData *data) {
        return "{\"type\":\"call\", \"call_id\":\"" + data->call_id + "\",\"from_number\":\"" + data->from_number + "\",\"from_name\":\"" + data->from_name +
         "\",\"direction\":\"" + data->direction + "\",\"auto_answer\":" + std::to_string(data->auto_answer) + "}";
    }

    static size_t writeCallback(char *contents, size_t size, size_t nmemb, void *userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }

    bool ServerImpl::UseFCM() const {
        return this->push_fcm_enabled;
    }

    bool ServerImpl::UseAPN() const {
        return this->push_apn_enabled;
    }

    int ServerImpl::PushWaitCallback() const {
        return this->push_wait_callback;
    }

    int ServerImpl::AutoAnswerDelayTime() const {
        return this->auto_answer_delay;
    }

    AsyncClientCall* ServerImpl::AsyncStreamPCMA(int64_t  domain_id, const char *uuid, const char *name, int32_t rate) {
        return amdClient_->Stream(domain_id, uuid, name, rate);
    }

    PushClient *ServerImpl::GetPushClient() {
        return pushClient;
    }

    static void split_str(const std::string& str, const std::string& delimiter, std::vector<std::string> &out) {
        size_t pos = 0;
        std::string s = str;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            out.push_back(s.substr(0, pos));
            s.erase(0, pos + delimiter.length());
        }

        out.push_back(s);
    }

    static switch_status_t wbt_tweaks_on_reporting(switch_core_session_t *session) {
        double talk = 0;
        switch_caller_profile_t *cp = nullptr;

        auto channel = switch_core_session_get_channel(session);
        auto profile = switch_channel_get_caller_profile(channel);

        for (cp = profile; cp; cp = cp->next) {
            if (cp->times && cp->times->bridged) {
                switch_time_t t2 = cp->times->transferred;
                if (t2 == 0) {
                    t2 = cp->times->hungup ? cp->times->hungup : switch_micro_time_now();
                }
                talk += difftime(t2,
                                 (switch_time_t)(cp->times->bridged));
            }
        }
        switch_channel_set_variable(channel, WBT_TALK_SEC, std::to_string(static_cast<int>(talk / 1000000)).c_str());
        return SWITCH_STATUS_SUCCESS;
    }

    static switch_status_t wbt_tweaks_on_init(switch_core_session_t *session) {
        if (heartbeat_interval) {
            auto channel = switch_core_session_get_channel(session);
            switch_core_session_enable_heartbeat(session, heartbeat_interval);
            switch_channel_set_variable_var_check(channel, "wbt_heartbeat", std::to_string(heartbeat_interval).c_str(), SWITCH_FALSE);
        }
        return SWITCH_STATUS_SUCCESS;
    }

    static PushData* get_push_body(const char *uuid, switch_channel *channel, int autoDelayTime) {
        auto *pData = new PushData;
        pData->call_id = std::string(uuid);
        const char *name = nullptr;
        const char *number = nullptr;
        const char *wbt_parent_id = switch_channel_get_variable(channel, "wbt_parent_id");
        const char *wbt_auto_answer = switch_channel_get_variable(channel, "wbt_auto_answer");

        if (wbt_parent_id) {
            pData->direction = "inbound";
            number = switch_channel_get_variable(channel, "wbt_from_number");
            name = switch_channel_get_variable(channel, "wbt_from_name");
        } else {
            pData->direction = "outbound";
            number = switch_channel_get_variable(channel, "Caller-Orig-Caller-ID-Number");
            name = switch_channel_get_variable(channel, "Caller-Orig-Caller-ID-Name");
            if (!number) {
                number = name = switch_channel_get_variable(channel, "wbt_destination");
            }
        }
        if (autoDelayTime > 0 && !switch_false(wbt_auto_answer)) {
            pData->auto_answer = autoDelayTime;
        } else {
            pData->auto_answer = 0;
        }


        pData->from_name = name ? std::string(name) : "";
        pData->from_number = number ? std::string(number) : "";

        return pData;
    }

    static inline void fire_event(switch_channel_t *channel, const char *name) {
        switch_event_t      *event;
        switch_status_t     status;
        status = switch_event_create_subclass(&event, SWITCH_EVENT_CLONE, name);
        if (status != SWITCH_STATUS_SUCCESS) {
            return;
        }
        switch_channel_event_set_data(channel, event);
        switch_event_fire(&event);
    }

    static inline void amd_fire_event(switch_channel_t *channel) {
        fire_event(channel, AMD_EVENT_NAME);
    }

    static inline void do_execute(switch_core_session_t *session, switch_channel_t *channel, const char *name) {
        char *arg = NULL;
        char *p;
        int bg = 0;
        const char *variable = switch_channel_get_variable(channel, name);
        char *expanded = NULL;
        char *app = NULL;

        if (!variable) {
            return;
        }

        expanded = switch_channel_expand_variables(channel, variable);
        app = switch_core_session_strdup(session, expanded);

        for(p = app; p && *p; p++) {
            if (*p == ' ' || (*p == ':' && (*(p+1) != ':'))) {
                *p++ = '\0';
                arg = p;
                break;
            } else if (*p == ':' && (*(p+1) == ':')) {
                bg++;
                break;
            }
        }

        switch_assert(app != NULL);
        if (!strncasecmp(app, "perl", 4)) {
            bg++;
        }

        if (bg) {
            switch_core_session_execute_application_async(session, app, arg);
        } else {
            switch_core_session_execute_application(session, app, arg);
        }
    }

    static switch_bool_t amd_read_audio_callback(switch_media_bug_t *bug, void *user_data, switch_abc_type_t type) {
        auto *ud = static_cast<Stream *>(user_data);

        switch (type) {
            case SWITCH_ABC_TYPE_INIT: {
                // connect
                try {
                    switch_core_session_get_read_impl(ud->session, &ud->read_impl);

                    auto var = switch_channel_get_variable(ud->channel, "wbt_ai_vad_threshold");
                    if (var) {
                        auto tmp = atoi(var);
                        if (tmp) {
                            ud->vad = switch_vad_init((int) ud->read_impl.actual_samples_per_second, 1);
                            ud->stop_vad_on_answer =
                                    switch_true(switch_channel_get_variable(ud->channel, "wbt_ai_vad_stop_on_answer")) == 1;
                            switch_vad_set_param(ud->vad, "thresh", tmp);
                            switch_log_printf(
                                    SWITCH_CHANNEL_SESSION_LOG(ud->session),
                                    SWITCH_LOG_DEBUG,
                                    "amd use vad thresh %d \n", tmp);

                            if ((var = switch_channel_get_variable(ud->channel, "wbt_ai_vad_debug"))) {
                                tmp = atoi(var);
                                if (tmp < 0) tmp = 0;

                                switch_vad_set_param(ud->vad, "debug", tmp);
                            };
                        }
                        var = switch_channel_get_variable(ud->channel, "wbt_vad_max_silence_sec");
                        if (var) {
                            tmp = atoi(var);
                            if (tmp) {
                                ud->max_silence_sec = tmp;
                                switch_log_printf(
                                        SWITCH_CHANNEL_SESSION_LOG(ud->session),
                                        SWITCH_LOG_DEBUG,
                                        "amd use vad maximum silence seconds %d \n", tmp);
                            }
                        }

                    }

                    if (ud->read_impl.actual_samples_per_second != MODEL_RATE) {
                        switch_resample_create(&ud->resampler,
                                               ud->read_impl.actual_samples_per_second,
                                               MODEL_RATE,
                                               320, SWITCH_RESAMPLE_QUALITY, 1);
                    } else {
                        ud->resampler = nullptr;
                    }
                } catch (...) {
                    switch_log_printf(
                            SWITCH_CHANNEL_SESSION_LOG(ud->session),
                            SWITCH_LOG_CRIT,
                            "GRPC stream: SWITCH_ABC_TYPE_INIT \n");
                }

                break;
            }

            case SWITCH_ABC_TYPE_CLOSE: {
                // cleanup
                try {
                    if (ud->resampler) {
                        switch_resample_destroy(&ud->resampler);
                    }

                    ud->client_->Finish();

                    std::string amd_result;
                    std::vector<std::string> amd_results(ud->client_->reply.results().begin(), ud->client_->reply.results().end());

                    bool skip_hangup = false;

                    if (ud->client_->reply.result().empty()) {
                        amd_result = "undefined";
                        if (ud->vad && switch_channel_test_flag(ud->channel, CF_ANSWERED)) {
                            amd_result = "silence";
                        }
//                        switch_channel_set_variable(ud->channel, "execute_on_answer", NULL); // TODO
                    } else {
                        amd_result = ud->client_->reply.result();
                    }

                    for (auto &l : ud->positive) {
                        if (l == amd_result) {
                            skip_hangup = true;
                            do_execute(ud->session, ud->channel, AMD_EXECUTE_VARIABLE);
                            break;
                        }
                    }

                    if (ud->vad) {
                        switch_vad_destroy(&ud->vad);
                    }

                    switch_channel_set_variable(ud->channel, WBT_AMD_AI, amd_result.c_str());
                    for (auto &r : amd_results) {
                        switch_channel_add_variable_var_check(ud->channel, WBT_AMD_AI_LOG, r.c_str(), SWITCH_FALSE, SWITCH_STACK_PUSH);
                    }

                    if (!amd_results.empty()) {
                        std::string s = std::accumulate(std::next(amd_results.begin()), amd_results.end(),
                                                        amd_results[0], // start with first element
                                                        [](std::string a, std::string &b) {
                                                            return std::move(a) + "<-" + b;
                                                        });
                        switch_log_printf(
                                SWITCH_CHANNEL_SESSION_LOG(ud->session),
                                SWITCH_LOG_NOTICE,
                                "ML amd result: %s [%s]\n", amd_result.c_str(), s.c_str());
                    }

                    auto ready = switch_channel_ready(ud->channel);
                    switch_channel_set_variable(ud->channel, WBT_AMD_AI_POSITIVE, !ready || !skip_hangup ? "false" : "true");
                    if (ready) {
                        amd_fire_event(ud->channel);
                    } else {
                        switch_log_printf(
                                SWITCH_CHANNEL_SESSION_LOG(ud->session),
                                SWITCH_LOG_WARNING,
                                "GRPC stream: skip amd event, channel state is hangup \n");
                    }
                    if (!skip_hangup) {
                        switch_channel_hangup(ud->channel, SWITCH_CAUSE_NORMAL_UNSPECIFIED);
                    }

                    delete ud->client_;
                    delete ud;
                } catch (...) {
                    switch_log_printf(
                            SWITCH_CHANNEL_SESSION_LOG(ud->session),
                            SWITCH_LOG_CRIT,
                            "GRPC stream: SWITCH_ABC_TYPE_CLOSE \n");
                }
                break;
            }

            case SWITCH_ABC_TYPE_READ:
            case SWITCH_ABC_TYPE_WRITE: {
                uint8_t data[SWITCH_RECOMMENDED_BUFFER_SIZE];
                switch_frame_t read_frame = { 0 };
                switch_status_t status;

                read_frame.data = data;
                read_frame.buflen = SWITCH_RECOMMENDED_BUFFER_SIZE;

                try {
                    status = switch_core_media_bug_read(bug, &read_frame, SWITCH_FALSE);
                    if (status != SWITCH_STATUS_SUCCESS && status != SWITCH_STATUS_BREAK) {
                        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(ud->session), SWITCH_LOG_DEBUG, "switch_core_media_bug_read SWITCH_TRUE \n");
                        return SWITCH_TRUE;
                    };

                    switch_vad_state_t vad_state = SWITCH_VAD_STATE_ERROR;

                    if (ud->vad) {
                        if (!ud->answered) {
                            ud->answered = switch_channel_test_flag(ud->channel, CF_ANSWERED) ;
                        }
                        if (ud->stop_vad_on_answer && ud->answered) {
                            switch_vad_destroy(&ud->vad);
                            ud->vad = nullptr;
                        } else {
                            vad_state = SWITCH_VAD_STATE_NONE;
                            vad_state = switch_vad_process(ud->vad, (int16_t *) read_frame.data,
                                                           read_frame.datalen / 2);
                            if (vad_state == SWITCH_VAD_STATE_STOP_TALKING) {
                                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(ud->session), SWITCH_LOG_DEBUG, "amd vad reset: %s\n",
                                                  switch_vad_state2str(vad_state));
                                switch_vad_reset(ud->vad);
                            }

                            if (ud->max_silence_sec) {
                                ud->frame_ms = 1000 / (ud->read_impl.actual_samples_per_second / read_frame.samples);
                                if (vad_state == SWITCH_VAD_STATE_NONE) {
                                    ud->silence_ms += ud->frame_ms;
                                } else {
                                    ud->silence_ms = 0;
                                }

                                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(ud->session), SWITCH_LOG_DEBUG, "amd vad state: %s [silence = %d]\n",
                                                  switch_vad_state2str(vad_state), ud->silence_ms / 1000);
                            }
                        }
                    }

                    if (ud->resampler) {
                        uint8_t resample_data[SWITCH_RECOMMENDED_BUFFER_SIZE];
                        auto data = (int16_t *) read_frame.data;
                        switch_resample_process(ud->resampler, data, (int) read_frame.datalen / 2);
                        auto linear_len = ud->resampler->to_len * 2;
                        memcpy(resample_data, ud->resampler->to, linear_len);
                        ud->client_->Write(resample_data, linear_len, vad_state);
                    } else {
                        ud->client_->Write(read_frame.data, read_frame.datalen, vad_state);
                    }

                    if (ud->client_->Finished()) {
                        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Finished\n");
                        return SWITCH_FALSE;
                    }

                    if (ud->max_silence_sec && ud->max_silence_sec <= (ud->silence_ms / 1000)) {
                        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(ud->session), SWITCH_LOG_DEBUG, "Maximum silence seconds\n");
                        return SWITCH_FALSE;
                    }

                } catch (...) {
                    switch_log_printf(
                            SWITCH_CHANNEL_SESSION_LOG(ud->session),
                            SWITCH_LOG_CRIT,
                            "GRPC stream: SWITCH_ABC_TYPE_WRITEs \n");
                }
                break;
            }
        }

        return SWITCH_TRUE;
    }

    #define WBT_AMD_SYNTAX "<positive labels>"
    SWITCH_STANDARD_APP(wbt_amd_function) {
        switch_channel_t *channel = switch_core_session_get_channel(session);
        switch_media_bug_t *bug = nullptr;
        switch_media_bug_flag_t flags = SMBF_READ_STREAM; //SMBF_WRITE_STREAM;
        int64_t domain_id = 0;
        std::vector<std::string> positive_labels;
        const char *err = nullptr;
        const char *tmp = nullptr;
        Stream *ud = nullptr;

        if (!server_->AllowAMDAi()) {
            err = "ai_disabled";
            goto error_;
        }

        if (!zstr(data)) {
            split_str(std::string(data), ",", positive_labels);
        } else {
            err = "ai_bad_request";
            goto error_;
        }

        tmp = switch_channel_get_variable(channel, "sip_h_X-Webitel-Domain-Id");
        if (!tmp || !(domain_id = atoi(tmp))) {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Can not stream session.  Not found sip_h_X-Webitel-Domain-Id\n");
            err = "ai_bad_request";
            goto error_;
        }

        if (!switch_channel_media_up(channel) || !switch_core_session_get_read_codec(session)) {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Can not stream session.  Media not enabled on channel\n");
            err = "ai_no_media";
            goto error_;
        }

        ud = new Stream;
        ud->session = session;
        ud->channel = channel;
        ud->positive = std::move(positive_labels);
        ud->client_ = server_->AsyncStreamPCMA(domain_id, switch_channel_get_uuid(channel), switch_channel_get_uuid(channel), MODEL_RATE);
        ud->vad = nullptr;
        ud->max_silence_sec = 0;
        ud->silence_ms = 0;

        if (!ud->client_) {
            err = "ai_create_client";
            goto error_;
        }

        if (switch_core_media_bug_add(
                session,
                BUG_STREAM_NAME,
                nullptr,
                amd_read_audio_callback,
                ud, //user_data
                0, //TODO
                flags,
                &bug) != SWITCH_STATUS_SUCCESS ) {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "Can not add media bug.  Media not enabled on channel\n");
            err = "ai_no_media";
            goto error_;
        }
        return;

        error_:
            // todo add positive application ?
            delete ud;

            switch_channel_set_variable(channel, WBT_AMD_AI_ERROR, err);
            switch_channel_set_variable(channel, "execute_on_answer", nullptr); // TODO
            amd_fire_event(channel);
            do_execute(session, channel, AMD_EXECUTE_VARIABLE);
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "AMD error code: %s\n", err);
    }

    SWITCH_STANDARD_APP(wbr_send_hook_function) {
        auto channel = switch_core_session_get_channel(session);
        auto dir = switch_channel_get_variable(channel, "sip_h_X-Webitel-Direction");
        if (!dir || strcmp(dir, "internal") != 0) {
            return;
        }
        auto uuid = switch_channel_get_uuid(channel);
        if (!uuid || !server_->GetPushClient()) {
            //todo
            return;
        }

        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "checking tweaks for %s\n", uuid);
        const char *wbt_push_fcm = switch_channel_get_variable(channel, "wbt_push_fcm");
        const char *wbt_push_apn = switch_channel_get_variable(channel, "wbt_push_apn");
        int send = 0;
        auto pData = get_push_body(uuid, channel, server_->AutoAnswerDelayTime());
        std::vector <std::string> android;
        std::vector <std::string> apple;

        if (wbt_push_fcm && server_->UseFCM()) {
            split_str(wbt_push_fcm, "::", android);
        }
        if (wbt_push_apn && server_->UseAPN()) {
            split_str(wbt_push_apn, "::", apple);
        }

        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "start push request %s\n", uuid);
        send = server_->GetPushClient()->Send(android, apple, session, server_->PushWaitCallback() + 2000, pData);
        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "stop push request %s [send count %d]\n", uuid, send);

        if (send && server_->PushWaitCallback() > 0) {
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "start wait callback %s [%d]\n", uuid, server_->PushWaitCallback());
            switch_channel_wait_for_flag(channel, CF_SLA_INTERCEPT, SWITCH_TRUE, server_->PushWaitCallback(), NULL); //10s
            switch_channel_clear_flag(channel, CF_SLA_INTERCEPT);
            switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "stop wait callback %s\n", uuid);
        }
        delete pData;
    }

    SWITCH_STANDARD_APP(wbr_queue_function) {
        switch_channel_t *channel = switch_core_session_get_channel(session);

        if (zstr(data)) {
            return;
        }

        while (switch_channel_ready(channel)) {
            switch_status_t pstatus = switch_ivr_play_file(session, NULL, data,
                                                           nullptr);

            if (pstatus == SWITCH_STATUS_BREAK || pstatus == SWITCH_STATUS_TIMEOUT) {
                break;
            }
        }
    }

    SWITCH_STANDARD_APP(wbt_queue_playback_function) {
        switch_channel_t *channel = switch_core_session_get_channel(session);
        switch_status_t status = SWITCH_STATUS_SUCCESS;
        const char *file = data;

        while (switch_channel_ready(channel)) {
            status = switch_ivr_play_file(session, NULL, file, NULL);

            if (status != SWITCH_STATUS_SUCCESS) {
                break;
            }
        }

        switch (status) {
            case SWITCH_STATUS_SUCCESS:
            case SWITCH_STATUS_BREAK:
                switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "FILE PLAYED");
                break;
            case SWITCH_STATUS_NOTFOUND:
                switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "FILE NOT FOUND");
                break;
            default:
                switch_channel_set_variable(channel, SWITCH_CURRENT_APPLICATION_RESPONSE_VARIABLE, "PLAYBACK ERROR");
                break;
        }

    }

    SWITCH_STANDARD_APP(wbt_blind_transfer_function) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "BLIND TRANSFER\n");
    }

    SWITCH_STANDARD_API(version_api_function) {
        stream->write_function(stream, "%s\n", MOD_BUILD_VERSION);
        stream->write_function(stream, "grpc: %s", grpc::Version().c_str());
        return SWITCH_STATUS_SUCCESS;
    }

    SWITCH_MODULE_LOAD_FUNCTION(mod_grpc_load) {
        try {
            *module_interface = switch_loadable_module_create_module_interface(pool, modname);
            auto config = loadConfig();
            switch_application_interface_t *app_interface;
            switch_api_interface_t *api_interface;
            heartbeat_interval = 0;
            if (config.heartbeat) {
                heartbeat_interval = config.heartbeat;
                wbt_state_handlers.on_init = wbt_tweaks_on_init;
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Use session heartbeat, %d seconds\n", heartbeat_interval);
            }
            switch_core_add_state_handler(&wbt_state_handlers);
            SWITCH_ADD_API(api_interface, "wbt_version", "Show build version", version_api_function, "");
            SWITCH_ADD_APP(app_interface, "wbt_queue", "wbt_queue", "wbt_queue", wbr_queue_function, "", SAF_NONE);
            SWITCH_ADD_APP(app_interface, "wbt_send_hook", "wbt_send_hook", "wbt_send_hook", wbr_send_hook_function, "", SAF_NONE | SAF_SUPPORT_NOMEDIA);
            SWITCH_ADD_APP(app_interface, "wbt_blind_transfer", "wbt_blind_transfer", "wbt_blind_transfer",
                           wbt_blind_transfer_function, "", SAF_NONE);
            SWITCH_ADD_APP(app_interface, "wbt_queue_playback", "wbt_queue_playback", "wbt_queue_playback",
                           wbt_queue_playback_function, "", SAF_NONE);
            SWITCH_ADD_APP(
                    app_interface,
                    BUG_STREAM_NAME,
                    BUG_STREAM_NAME,
                    BUG_STREAM_NAME,
                    wbt_amd_function,
                    WBT_AMD_SYNTAX,
                    SAF_NONE);

            switch_event_reserve_subclass("SWITCH_EVENT_CUSTOM::" EVENT_NAME);

            server_ = new ServerImpl(config);

            server_->Run();
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Module loaded completed FCM=%d APN=%d\n", server_->UseFCM(), server_->UseAPN());
            return SWITCH_STATUS_SUCCESS;
        } catch (std::string &err) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error loading GRPC module: %s\n",
                              err.c_str());
            return SWITCH_STATUS_GENERR;
        } catch (std::exception &ex) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error loading GRPC module: %s\n",
                              ex.what());
            return SWITCH_STATUS_GENERR;
        } catch (...) { // Exceptions must not propogate to C caller
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error loading GRPC module\n");
            return SWITCH_STATUS_GENERR;
        }

    }

    SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_grpc_shutdown) {
        try {
            switch_event_free_subclass("SWITCH_EVENT_CUSTOM::" EVENT_NAME);
            switch_core_remove_state_handler(&wbt_state_handlers);
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

    Stream::~Stream() {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Destroy Stream\n");
    }
}
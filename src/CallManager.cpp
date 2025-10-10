//
// Created by root on 24.06.19.
//

#include "CallManager.h"

mod_grpc::CallManager::CallManager() {
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_CREATE, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_ANSWER, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_HOLD, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_UNHOLD, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_DTMF, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_BRIDGE, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_HANGUP_COMPLETE, nullptr, CallManager::handle_call_event,
                      nullptr);
    //    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_TALK, nullptr, CallManager::handle_call_event, nullptr);
    //    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_NOTALK, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_RECORD_START, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_RECORD_STOP, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_SESSION_HEARTBEAT, nullptr, CallManager::handle_call_event,
                      nullptr);

    //    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_EXECUTE, nullptr, CallManager::handle_call_event, nullptr);

    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CUSTOM, AMD_EVENT_NAME, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CUSTOM, EAVESDROP_EVENT_NAME, CallManager::handle_call_event,
                      nullptr);
    auto status = switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CUSTOM, TRANSCRIBE_EVENT_END_OF_TRANSCRIPT,
                                    CallManager::handle_call_event, nullptr);
    if (status != SWITCH_STATUS_SUCCESS) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Unhandled bind event: %s\n",
                          TRANSCRIBE_EVENT_END_OF_TRANSCRIPT);
    }
    //    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CUSTOM, VALET_PARK_NAME, CallManager::handle_call_event, nullptr);
}

mod_grpc::CallManager::~CallManager() {
    switch_event_unbind_callback(CallManager::handle_call_event);
}

void mod_grpc::CallManager::handle_call_event(switch_event_t *event) {
    try {
        if (event->event_id != SWITCH_EVENT_CHANNEL_HANGUP_COMPLETE && switch_false(
                switch_event_get_header(event, "variable_" SKIP_EVENT_VARIABLE))) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Skip event %s by variable\n",
                              switch_event_name(event->event_id));
            return;
        }
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Receive event: [%s]\n",
                          switch_event_name(event->event_id));
        switch (event->event_id) {
            case SWITCH_EVENT_CHANNEL_CREATE: {
                auto contact = get_str(switch_event_get_header(event, "variable_sip_h_X-Webitel-Contact-Id"));
                if (!contact.empty()) {
                    auto uuid_ = get_str(switch_event_get_header(event, "Unique-ID"));
                    auto session = switch_core_session_locate(uuid_.c_str());
                    if (session) {
                        auto channel = switch_core_session_get_channel(session);
                        switch_channel_set_variable(channel, "wbt_contact_id", contact.c_str());
                        switch_core_session_rwunlock(session);
                        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "variable_wbt_contact_id",
                                                       contact.c_str());
                    }
                }

                CallEvent<Ringing>(event).fire();
                break;
            }

            case SWITCH_EVENT_CHANNEL_ANSWER: {
                CallEvent<Active>(event).fire();
                break;
            }
            case SWITCH_EVENT_CHANNEL_UNHOLD: {
                auto uuid_ = get_str(switch_event_get_header(event, "Unique-ID"));
                auto session = switch_core_session_locate(uuid_.c_str());
                if (session) {
                    auto channel = switch_core_session_get_channel(session);
                    if (!switch_channel_test_flag(channel, CF_HANGUP_HELD) && !switch_channel_test_flag(
                            channel, CF_PROTO_HOLD)) {
                        CallEvent<Active>(event).fire();
                    }
                    switch_core_session_rwunlock(session);
                }
                break;
            }

            case SWITCH_EVENT_CHANNEL_BRIDGE:
                CallEvent<Bridge>(event).fire();
                break;

            case SWITCH_EVENT_DTMF: {
                auto uuid_ = get_str(switch_event_get_header(event, "Unique-ID"));
                auto session = switch_core_session_locate(uuid_.c_str());
                if (session) {
                    auto channel = switch_core_session_get_channel(session);
                    auto domain_id_ = switch_channel_get_variable(channel, "sip_h_X-Webitel-Domain-Id");
                    auto user_id_ = switch_channel_get_variable(channel, "sip_h_X-Webitel-User-Id");
                    switch_core_session_rwunlock(session);
                    if (domain_id_) {
                        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "variable_sip_h_X-Webitel-Domain-Id",
                                                       domain_id_);
                    }
                    if (user_id_) {
                        switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "variable_sip_h_X-Webitel-User-Id",
                                                       user_id_);
                    }
                }
                CallEvent<DTMF>(event).fire();
                break;
            }

            case SWITCH_EVENT_CHANNEL_HOLD:
                //todo switch_ivr_record_session_mask  / unmask ?
                CallEvent<Hold>(event).fire();
                break;

            case SWITCH_EVENT_CHANNEL_HANGUP_COMPLETE:
                CallEvent<Hangup>(event).fire();
                break;

            case SWITCH_EVENT_TALK:
                CallEvent<Voice>(event).fire();
                break;

            case SWITCH_EVENT_NOTALK:
                CallEvent<Silence>(event).fire();
                break;

            case SWITCH_EVENT_SESSION_HEARTBEAT:
                CallEvent<Heartbeat>(event).fire();
                break;

            case SWITCH_EVENT_CHANNEL_EXECUTE:
                CallEvent<Execute>(event).fire();
                break;

            case SWITCH_EVENT_RECORD_START: {
                auto uuid_ = get_str(switch_event_get_header(event, "Unique-ID"));
                auto session = switch_core_session_locate(uuid_.c_str());
                if (session) {
                    auto channel = switch_core_session_get_channel(session);
                    auto st = switch_channel_get_variable(channel, RECORD_SESSION_START_NAME);
                    if (!st) {
                        switch_channel_set_variable(channel, RECORD_SESSION_START_NAME,
                                                    std::to_string(unixTimestamp()).c_str());
                    }
                    switch_core_session_rwunlock(session);
                }
                break;
            }

            case SWITCH_EVENT_RECORD_STOP: {
                auto uuid_ = get_str(switch_event_get_header(event, "Unique-ID"));
                auto session = switch_core_session_force_locate(uuid_.c_str());
                if (session) {
                    switch_channel_set_variable(switch_core_session_get_channel(session), RECORD_SESSION_STOP_NAME,
                                                std::to_string(unixTimestamp()).c_str());
                    switch_core_session_rwunlock(session);
                }
                break;
            }

            case SWITCH_EVENT_CUSTOM:
                if (strcmp(AMD_EVENT_NAME, event->subclass_name) == 0) {
                    CallEvent<AMD>(event).fire();
                } else if (strcmp(EAVESDROP_EVENT_NAME, event->subclass_name) == 0) {
                    CallEvent<Eavesdrop>(event).fire();
                } else if (strcmp(TRANSCRIBE_EVENT_END_OF_TRANSCRIPT, event->subclass_name) == 0) {
                    CallEvent<Transcript>(event).fire();
                } else if (strcmp(VALET_PARK_NAME, event->subclass_name) == 0) {
                    auto action = switch_event_get_header(event, "Action");
                    if (strcmp(action, "hold") == 0) {
                        CallEvent<JoinQueue>(event).fire();
                    } else if (strcmp(action, "bridge") == 0) {
                    } else if (strcmp(action, "exit") == 0) {
                        CallEvent<LeavingQueue>(event).fire();
                    }
                } else {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Unhandled custom event: %s\n",
                                      event->subclass_name);
                }
                break;
            default:
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Unhandled event %s\n",
                                  switch_event_name(event->event_id));
                break;
        }
    } catch (std::exception &e) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Event %s\n", e.what());
    }
}

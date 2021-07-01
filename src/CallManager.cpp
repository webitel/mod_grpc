//
// Created by root on 24.06.19.
//

#include "CallManager.h"

#define CALL_MANAGER_NAME "CALL_MANAGER"
#define VALET_PARK_NAME "valet_parking::info"
#define AMD_EVENT_NAME "amd::info"
#define SKIP_EVENT_VARIABLE "skip_channel_events"

mod_grpc::CallManager::CallManager() {
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_CREATE, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_ANSWER, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_HOLD, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_UNHOLD, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_DTMF, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_BRIDGE, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_HANGUP_COMPLETE, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_TALK, nullptr, CallManager::handle_call_event, nullptr);
    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_NOTALK, nullptr, CallManager::handle_call_event, nullptr);

//    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CHANNEL_EXECUTE, nullptr, CallManager::handle_call_event, nullptr);

    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CUSTOM, AMD_EVENT_NAME, CallManager::handle_call_event, nullptr);
//    switch_event_bind(CALL_MANAGER_NAME, SWITCH_EVENT_CUSTOM, VALET_PARK_NAME, CallManager::handle_call_event, nullptr);
}

mod_grpc::CallManager::~CallManager() {
    switch_event_unbind_callback(CallManager::handle_call_event);
}

void mod_grpc::CallManager::handle_call_event(switch_event_t *event) {
    try {
        if (switch_event_get_header(event, SKIP_EVENT_VARIABLE)) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Skip event %s by variable\n", switch_event_name(event->event_id));
        }
        switch (event->event_id) {
            case SWITCH_EVENT_CHANNEL_CREATE:
                CallEvent<Ringing>(event).fire();
                break;

            case SWITCH_EVENT_CHANNEL_ANSWER:
            case SWITCH_EVENT_CHANNEL_UNHOLD:
                CallEvent<Active>(event).fire();
                break;

            case SWITCH_EVENT_CHANNEL_BRIDGE:
                CallEvent<Bridge>(event).fire();
                break;

            case SWITCH_EVENT_DTMF:
                CallEvent<DTMF>(event).fire();
                break;

            case SWITCH_EVENT_CHANNEL_HOLD:
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

            case SWITCH_EVENT_CHANNEL_EXECUTE:
                CallEvent<Execute>(event).fire();
                break;

            case SWITCH_EVENT_CUSTOM:
                if (strcmp(AMD_EVENT_NAME, event->subclass_name) == 0) {
                    CallEvent<AMD>(event).fire();
                } else if (strcmp(VALET_PARK_NAME, event->subclass_name) == 0) {
                    auto action = switch_event_get_header(event, "Action");
                    if (strcmp(action, "hold") == 0) {
                        CallEvent<JoinQueue>(event).fire();
                    } else if (strcmp(action, "bridge") == 0) {

                    } else if (strcmp(action, "exit") == 0) {
                        CallEvent<LeavingQueue>(event).fire();
                    }
                } else {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Unhandled custom event: %s\n", event->subclass_name);
                }
                break;
            default:
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Unhandled event %s\n", switch_event_name(event->event_id));
                break;
        }
    } catch (std::exception& e) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "Event %s\n", e.what());
    }
}
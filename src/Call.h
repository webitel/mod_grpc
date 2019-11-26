//
// Created by root on 11/22/19.
//

#ifndef MOD_GRPC_CALL_H
#define MOD_GRPC_CALL_H

#include <bits/unique_ptr.h>
#include <iostream>

extern "C" {
#include <switch.h>
}

#define EVENT_NAME "WEBITEL_CALL"
#define HEADER_NAME_NODE_NAME "node_name"
#define HEADER_NAME_ID "id"
#define HEADER_NAME_USER_ID "user_id"
#define HEADER_NAME_DOMAIN_ID "domain_id"
#define HEADER_NAME_HANGUP_CAUSE "cause"
#define HEADER_NAME_ACTION "action"
#define HEADER_NAME_DIRECTION "direction"
#define HEADER_NAME_DESTINATION "destination"
#define HEADER_NAME_FROM_ID "from_number"
#define HEADER_NAME_FROM_NAME "from_name"
#define HEADER_NAME_TO_ID "to_number"
#define HEADER_NAME_TO_NAME "to_name"
#define HEADER_NAME_DTMF_DIGIT "digit"
#define HEADER_NAME_PARENT_ID "parent_id"
#define HEADER_NAME_GATEWAY_ID "gateway_id"

#define get_str(c) c ? std::string(c) : std::string()

enum CallActions { Ringing, Active, Bridge, Hold, DTMF, Voice, Silence, Execute, Update, Hangup };

//TODO
static const char* callEventStr(CallActions e) {
    switch (e) {
        case Ringing:
            return "ringing";
        case Active:
            return "active";
        case Bridge:
            return "bridge";
        case Hold:
            return "hold";
        case DTMF:
            return "dtmf";
        case Voice:
            return "voice";
        case Silence:
            return "silence";
        case Execute:
            return "execute";
        case Update:
            return "update";
        case Hangup:
            return "hangup";
        default:
            return "unknown";
    }
}

class BaseCallEvent {
public:
    std::string uuid_;
    std::string node_;
    std::string domain_id_;
    std::string user_id_;

    explicit BaseCallEvent(CallActions action, switch_event_t *e) {
        if (switch_event_create_subclass(&out, SWITCH_EVENT_CLONE, EVENT_NAME) != SWITCH_STATUS_SUCCESS) {
            throw std::overflow_error("Couldn't create event\n");
        }
        uuid_ = get_str(switch_event_get_header(e, "Unique-ID"));
        node_ = get_str(switch_event_get_header(e, "FreeSWITCH-Switchname"));
        domain_id_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-Domain-Id"));
        user_id_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-User-Id"));

        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_ACTION, callEventStr(action));
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_ID, uuid_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_NODE_NAME, node_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_DOMAIN_ID, domain_id_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_USER_ID, user_id_.c_str());
    }

    ~BaseCallEvent() {
        switch_event_destroy(&out);
    }

    void fire() {
//        DUMP_EVENT(out)
        switch_event_fire(&out);
    }

protected:
    switch_event_t *out = nullptr;
    void set_call_info(switch_event_t *e) {
        const char *tmp = nullptr;
        const char *displayDirection = switch_event_get_header(e, "variable_sip_h_X-Webitel-Display-Direction");
        const char *direction = switch_event_get_header(e, "variable_sip_h_X-Webitel-Direction");

        bool answered(false);
        std::string direction_;
        std::string destination_;
        std::string from_number_;
        std::string from_name_;
        std::string to_number_;
        std::string to_name_;
        std::string parent_id_;
        std::string gateway_id_;

        if (displayDirection && strlen(displayDirection) != 0) {
            direction_ = std::string(displayDirection);
        } else if (direction && strcmp(direction, "inbound") == 0) {
            direction_ = "inbound";
        } else {
            direction_ = "outbound";
        }

        parent_id_ = get_str(switch_event_get_header(e, "Other-Leg-Unique-ID"));
        gateway_id_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-Gateway-Id"));

        if ((tmp = switch_event_get_header(e, "Caller-Channel-Answered-Time") ) && strcmp(tmp, "0") != 0) {
            answered = true;
        }

        if ( direction_ == "outbound" && direction && strcmp(direction, "internal") == 0 &&
             (!answered || e->event_id == SWITCH_EVENT_CHANNEL_ANSWER )) {

            destination_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-Destination"));

            from_number_ = get_str(switch_event_get_header(e, "Caller-Callee-ID-Number"));
            from_name_ = get_str(switch_event_get_header(e, "Caller-Callee-ID-Name"));

            to_number_ = get_str(switch_event_get_header(e, "Caller-Caller-ID-Number"));
            to_name_ = get_str(switch_event_get_header(e, "Caller-Caller-ID-Name"));
        } else {
            destination_ = get_str(switch_event_get_header(e, "Caller-Destination-Number"));

            from_number_ = get_str(switch_event_get_header(e, "Caller-Caller-ID-Number"));
            from_name_ = get_str(switch_event_get_header(e, "Caller-Caller-ID-Name"));

            to_number_ = get_str(switch_event_get_header(e, "Caller-Callee-ID-Number"));
            to_name_ = get_str(switch_event_get_header(e, "Caller-Callee-ID-Name"));
        }

        if (to_number_.empty()) {
            to_number_ = destination_;
        }
        if (to_name_.empty()) {
            to_name_ = to_number_;
        }

        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_DIRECTION, direction_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_DESTINATION, destination_.c_str());

        if (!parent_id_.empty()) {
            switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_PARENT_ID, parent_id_.c_str());
        }
        if (!gateway_id_.empty()) {
            switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_GATEWAY_ID, gateway_id_.c_str());
        }

        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_FROM_ID, from_number_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_FROM_NAME, from_name_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_TO_ID, to_number_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_TO_NAME, to_name_.c_str());
        set_data(e);
    }

    static bool prefix(const char *pre, const char *str) {
        return strncmp(str, pre, strlen(pre)) == 0;
    }

    void set_data (switch_event_t *event) {
        switch_event_header_t *hp;
        cJSON *cj;
        bool found(false);

        cj = cJSON_CreateObject();

        for (hp = event->headers; hp; hp = hp->next) {

            if (!prefix("variable_wbt_", hp->name) ) {
                continue;
            }
            found = true;

            auto name = std::string(hp->name);
            name = name.substr(13, name.length());

            if (hp->idx) {
                cJSON *a = cJSON_CreateArray();
                int i;

                for(i = 0; i < hp->idx; i++) {
                    cJSON_AddItemToArray(a, cJSON_CreateString(hp->array[i]));
                }

                cJSON_AddItemToObject(cj, name.c_str(), a);

            } else {
                cJSON_AddItemToObject(cj, name.c_str(), cJSON_CreateString(hp->value));
            }
        }

        if (found) {
            switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, "payload", cJSON_PrintUnformatted(cj));
        }

        cJSON_Delete(cj);
    }
};


template <CallActions F> class CallEvent {
public:
    CallEvent() = default;
};

template <> class CallEvent<Ringing> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Ringing, e) {
        set_call_info(e);
    };

};

template <> class CallEvent<Active> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Active, e) {

    };
};

template <> class CallEvent<Bridge> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Bridge, e) {
        set_call_info(e);
    };
};

template <> class CallEvent<Hold> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Hold, e) {

    };
};

template <> class CallEvent<Voice> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Voice, e) {

    };
};

template <> class CallEvent<Silence> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Silence, e) {

    };
};

template <> class CallEvent<DTMF> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(DTMF, e) {

        auto digit =  get_str(switch_event_get_header(e, "DTMF-Digit"));
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_DTMF_DIGIT, digit.c_str());
    };
};

template <> class CallEvent<Update> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Update, e) {
        std::cout << "Update" << this->uuid_ << std::endl;
    };
};

template <> class CallEvent<Hangup> : public BaseCallEvent {
public:
    std::string cause_;
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Hangup, e) {
        cause_ = get_str(switch_event_get_header(e, "variable_hangup_cause"));
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_HANGUP_CAUSE, cause_.c_str());
    };
};

template <> class CallEvent<Execute> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Execute, e) {
        auto app_ = get_str(switch_event_get_header(e, "Application"));
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, "application", app_.c_str());
    };
};


#endif //MOD_GRPC_CALL_H

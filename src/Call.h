//
// Created by root on 11/22/19.
//

#ifndef MOD_GRPC_CALL_H
#define MOD_GRPC_CALL_H

#include <bits/unique_ptr.h>
#include <map>
#include <iterator>
#include <ctime>
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
#define HEADER_NAME_OWNER_ID "owner_id"
#define HEADER_NAME_GATEWAY_ID "gateway_id"
#define HEADER_NAME_ACTIVITY_AT "activity_at"
#define HEADER_NAME_VIDEO_FLOW "video_flow"
#define HEADER_NAME_VIDEO_REQUEST "video_request"
#define HEADER_NAME_SCREEN_REQUEST "screen_request"
#define HEADER_NAME_QUEUE_NODE "queue_node"
#define HEADER_NAME_DATA "data"

#define get_str(c) c ? std::string(c) : std::string()

enum CallActions { Ringing, Active, Bridge, Hold, DTMF, Voice, Silence, Execute, Update, JoinQueue, LeavingQueue, Hangup };

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
        case JoinQueue:
            return "join_queue";
        case LeavingQueue:
            return "leaving_queue";
        default:
            return "unknown";
    }
}

static long int unixTimestamp() {
    return static_cast<long int> (std::time(nullptr)) * 1000; //FIXME
}

class BaseCallEvent {
public:
    std::string uuid_;
    std::string node_;
    std::string domain_id_;
    std::string user_id_;
    std::string cc_node_;

    explicit BaseCallEvent(CallActions action, switch_event_t *e) {
        if (switch_event_create_subclass(&out, SWITCH_EVENT_CLONE, EVENT_NAME) != SWITCH_STATUS_SUCCESS) {
            throw std::overflow_error("Couldn't create event\n");
        }
        body_ = cJSON_CreateObject();

        uuid_ = get_str(switch_event_get_header(e, "Unique-ID"));
        node_ = get_str(switch_event_get_header(e, "FreeSWITCH-Switchname"));
        domain_id_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-Domain-Id"));
        user_id_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-User-Id"));
        cc_node_ = get_str(switch_event_get_header(e, "variable_cc_node_id"));

        if (!cc_node_.empty()) {
            switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_QUEUE_NODE, cc_node_.c_str());
        }

        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_ACTION, callEventStr(action));
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_ID, uuid_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_NODE_NAME, node_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_DOMAIN_ID, domain_id_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_USER_ID, user_id_.c_str()); // FIXME
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_ACTIVITY_AT, std::to_string(unixTimestamp()).c_str());
    }

    ~BaseCallEvent() {
        cJSON_Delete(body_);
        switch_event_destroy(&out);
    }

    void addAttribute(const char *header, const std::string &val) {
        cJSON_AddItemToObject(body_, header, cJSON_CreateString(val.c_str()));
    }

    void addAttribute(const char *header, const bool &val) {
        cJSON_AddItemToObject(body_, header, cJSON_CreateBool(val));
    }

    void addAttribute(const char *header, const double &number) {
        cJSON_AddItemToObject(body_, header, cJSON_CreateNumber(number));
    }

    void addAttribute(const char *header, cJSON *attr) {
        cJSON_AddItemToObject(body_, header, attr);
    }

    void fire() {
        if (body_->child) {
            switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_DATA, cJSON_PrintUnformatted(body_));
        }
        DUMP_EVENT(out)
        switch_event_fire(&out);
    }

protected:
    switch_event_t *out = nullptr;
    cJSON *body_ = nullptr;

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
        std::string video_flow_;
        std::string video_request_;
        std::string screen_request_;
        std::string owner_id_;

        if (displayDirection && strlen(displayDirection) != 0) {
            direction_ = std::string(displayDirection);
        } else if (direction && ( strcmp(direction, "inbound") == 0 || strcmp(direction, "internal") == 0)) {
            direction_ = "inbound";
        } else {
            direction_ = "outbound";
        }

        parent_id_ = get_str(switch_event_get_header(e, "Other-Leg-Unique-ID"));
        owner_id_ = get_str(switch_event_get_header(e, "variable_request_parent_call_id"));

        gateway_id_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-Gateway-Id"));
        video_flow_ = get_str(switch_event_get_header(e, "variable_video_media_flow"));
        video_request_ = get_str(switch_event_get_header(e, "variable_video_request"));
        screen_request_ = get_str(switch_event_get_header(e, "variable_screen_request"));

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

        addAttribute(HEADER_NAME_DIRECTION, direction_);
        addAttribute(HEADER_NAME_DESTINATION, destination_);

        addAttribute(HEADER_NAME_FROM_ID, from_number_);
        addAttribute(HEADER_NAME_FROM_NAME, from_name_);
        addAttribute(HEADER_NAME_TO_ID, to_number_);
        addAttribute(HEADER_NAME_TO_NAME, to_name_);

        if (!video_flow_.empty()) {
            addAttribute(HEADER_NAME_VIDEO_FLOW, video_flow_);
        }

        if (!answered && video_request_ == "true") {
            addAttribute(HEADER_NAME_VIDEO_REQUEST, "true");
        }

        if (!answered && screen_request_ == "true") {
            addAttribute(HEADER_NAME_SCREEN_REQUEST, "true");
        }

        if (!parent_id_.empty()) {
            addAttribute(HEADER_NAME_PARENT_ID, parent_id_);
        }

        if (!owner_id_.empty()) {
            addAttribute(HEADER_NAME_OWNER_ID, owner_id_);
        }

        if (!gateway_id_.empty()) {
            addAttribute(HEADER_NAME_GATEWAY_ID, gateway_id_);
        }

        set_data(e);
    }

    static bool prefix(const char *pre, const char *str) {
        return strncmp(str, pre, strlen(pre)) == 0;
    }

    void set_queue_data(switch_event_t *event) {
        switch_event_header_t *hp;
        cJSON *cj;
        bool found(false);

        cj = cJSON_CreateObject();

        for (hp = event->headers; hp; hp = hp->next) {

            if (!prefix("variable_cc_", hp->name) ) {
                continue;
            }
            found = true;

            auto name = std::string(hp->name);
            name = name.substr(12, name.length());

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
            addAttribute("queue", cj);
        }
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
            addAttribute("payload", cj);
        }
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
        addAttribute(HEADER_NAME_DTMF_DIGIT, digit);
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
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Hangup, e) {
        auto cause_ = get_str(switch_event_get_header(e, "variable_hangup_cause"));
        auto sip_code_ = switch_event_get_header(e, "variable_sip_term_status");

        addAttribute(HEADER_NAME_HANGUP_CAUSE, cause_);
        addAttribute("originate_success",
                switch_event_get_header(e, "variable_grpc_originate_success") != nullptr);

        if (sip_code_) {
            addAttribute("sip", std::atof(sip_code_));
        }

    };
};

template <> class CallEvent<Execute> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Execute, e) {
        auto app_ = get_str(switch_event_get_header(e, "Application"));
        addAttribute("application", app_);
    };
};

template <> class CallEvent<JoinQueue> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(JoinQueue, e) {
        set_call_info(e);
        set_queue_data(e);
    };
};

template <> class CallEvent<LeavingQueue> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(LeavingQueue, e) {

    };
};


#endif //MOD_GRPC_CALL_H

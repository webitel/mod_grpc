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
#include "utils.h"

extern "C" {
#include <switch.h>
#include <switch_utils.h>
}

#define EVENT_NAME "WEBITEL_CALL"
#define HEADER_NAME_NODE_NAME "app_id"
#define HEADER_NAME_ID "id"
#define HEADER_NAME_USER_ID "user_id"
#define HEADER_NAME_GATEWAY_ID "gateway_id"
#define HEADER_NAME_DOMAIN_ID "domain_id"
#define HEADER_NAME_HANGUP_CAUSE "cause"
#define HEADER_NAME_EVENT "event"
#define HEADER_NAME_DTMF_DIGIT "digit"
#define HEADER_NAME_TIMESTAMP "timestamp"
#define HEADER_NAME_CC_NODE "cc_app_id"
#define HEADER_NAME_DATA "data"

#define CALL_MANAGER_NAME "CALL_MANAGER"
#define VALET_PARK_NAME "valet_parking::info"
#define AMD_EVENT_NAME "amd::info"
#define EAVESDROP_EVENT_NAME "eavesdrop::info"
#define TRANSCRIBE_EVENT_END_OF_TRANSCRIPT "google_transcribe::is_final"
#define SKIP_EVENT_VARIABLE "process_cdr"
#define RECORD_SESSION_START_NAME  "wbt_start_record"
#define RECORD_SESSION_STOP_NAME  "wbt_stop_record"
#define WBT_TALK_SEC  "wbt_talk_sec"
#define WBT_AMD_AI  "wbt_amd_ai"
#define WBT_AMD_AI_LOG  "wbt_amd_ai_log"
#define WBT_AMD_AI_ERROR  "wbt_amd_ai_error"
#define WBT_AMD_AI_POSITIVE  "wbt_amd_ai_positive"
#define WBT_EAVESDROP_STATE  "wbt_eavesdrop_state"

#define get_str(c) c ? std::string(c) : std::string()

enum CallActions { Ringing, Active, Bridge, Hold, DTMF, Voice, Silence, Execute, Update, JoinQueue, LeavingQueue, AMD, Hangup, Eavesdrop, Heartbeat, Transcript };

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
        case AMD:
            return "amd";
        case Eavesdrop:
            return "eavesdrop";
        case Heartbeat:
            return "heartbeat";
        case Transcript:
            return "transcript";
        default:
            return "unknown";
    }
}

class CallEndpoint {
public:
    std::string type;
    std::string id;
    std::string name;
    std::string number;
};

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
        event_ = new Event(e);
        e_ = e;
        body_ = cJSON_CreateObject();
        parent_ = switch_event_get_header(e_, "variable_wbt_parent_id");

        uuid_ = get_str(switch_event_get_header(e, "Unique-ID"));
        node_ = get_str(switch_event_get_header(e, "FreeSWITCH-Switchname"));
        domain_id_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-Domain-Id"));
        user_id_ = get_str(switch_event_get_header(e, "variable_sip_h_X-Webitel-User-Id"));
        cc_node_ = get_str(switch_event_get_header(e, "variable_cc_app_id"));

        if (!cc_node_.empty()) {
            switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_CC_NODE, cc_node_.c_str());
        }

        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_EVENT, callEventStr(action));
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_ID, uuid_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_NODE_NAME, node_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_DOMAIN_ID, domain_id_.c_str());
        switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_TIMESTAMP, std::to_string(mod_grpc::unixTimestamp()).c_str());

        if (!user_id_.empty()) {
            switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_USER_ID, user_id_.c_str());
            addAttribute(HEADER_NAME_USER_ID, static_cast<double>(std::stoi(user_id_)));
        }
    }

    ~BaseCallEvent() {
        e_ = nullptr;
        cJSON_Delete(body_);
        switch_event_destroy(&out);
        delete event_;
    }

    void addAttribute(const char *header, const std::string &val) {
        cJSON_AddItemToObject(body_, header, cJSON_CreateString(val.c_str()));
    }

    void addAttribute(const char *header, const char *val) {
        cJSON_AddItemToObject(body_, header, cJSON_CreateString(val));
    }

    void addAttribute(const char *header, const bool &val) {
        cJSON_AddItemToObject(body_, header, cJSON_CreateBool(val));
    }

    void addAttribute(const char *header, const double &number) {
        cJSON_AddItemToObject(body_, header, cJSON_CreateNumber(number));
    }

    void addAttribute(const char *header, const int &number) {
        cJSON_AddItemToObject(body_, header, cJSON_CreateNumber(number));
    }

    void addAttribute(const char *header, cJSON *attr) {
        cJSON_AddItemToObject(body_, header, attr);
    }

    void addArrayValue(switch_event_header_t *e, const char *var_name, bool number) {
        cJSON *arr = cJSON_CreateArray();
        if (e->idx) {
            for (int i = 0; i < e->idx; i++) {
                cJSON_AddItemToArray(arr, number ? cJSON_CreateNumber(std::atof(e->array[i])) : cJSON_CreateString(e->array[i]));
            }
        } else if (e->value) {
            cJSON_AddItemToArray(arr, number ? cJSON_CreateNumber(std::atof(e->value)) : cJSON_CreateString(e->value));
        }
        addAttribute(var_name, arr);
    }

    void notifyEavesdropPartner(const std::string& type) {
        auto agentCallId = event_->getVar("variable_wbt_eavesdrop_agent_id");
        if (!agentCallId.empty()) {
            switch_core_session_t *other_session;
            other_session = switch_core_session_locate(agentCallId.c_str());
            if (other_session) {
                switch_event_t *other_event;
                if (switch_event_create_subclass(&other_event, SWITCH_EVENT_CUSTOM, EAVESDROP_EVENT_NAME) == SWITCH_STATUS_SUCCESS) {
                    switch_channel_event_set_data(switch_core_session_get_channel(other_session), other_event);
                    switch_event_add_header_string(other_event, SWITCH_STACK_BOTTOM, "variable_" WBT_EAVESDROP_STATE, eavesdropStateName().c_str());
                    switch_event_add_header_string(other_event, SWITCH_STACK_BOTTOM, "variable_wbt_eavesdrop_type", type.c_str());
                }
                switch_core_session_rwunlock(other_session);

                if (other_event) {
                    switch_event_fire(&other_event);
                    free(other_event);
                }
            }
        }
    }

    void fire() {
        char *b = nullptr;
        if (body_->child) {
            b = cJSON_PrintUnformatted(body_);
            switch_event_add_header_string(out, SWITCH_STACK_BOTTOM, HEADER_NAME_DATA, b);
        }
//        DUMP_EVENT(out)
        switch_event_fire(&out);
        if (b) {
            cJSON_free(b);
        }
    }

protected:
    class Event {
    public:
        explicit Event(switch_event_t *e) {
            e_ = e;
        }
        ~Event() {
            e_ = nullptr;
        }
        inline std::string getVar(const char *name) {
            return get_str(switch_event_get_header(e_, name));
        }

    private:
        switch_event_t *e_ = nullptr;
    };

    switch_event_t *out = nullptr;
    cJSON *body_ = nullptr;
    switch_event_t *e_ = nullptr;
    Event *event_ = nullptr;
    const char *parent_ = nullptr;

    struct Info {
        CallEndpoint *from = nullptr;
        CallEndpoint *to = nullptr;
        std::string parent_id;
        std::string direction;
        std::string destination;
    };

    struct OutboundCallParameters {
        bool Video;
        bool Screen;
        std::string AutoAnswer;
        bool DisableStun;
    };

    static cJSON* toJson(CallEndpoint *e) {
        auto j = cJSON_CreateObject();
        cJSON_AddItemToObject(j, "type", cJSON_CreateString(e->type.c_str()));
        cJSON_AddItemToObject(j, "number", cJSON_CreateString(e->number.c_str()));
        cJSON_AddItemToObject(j, "name", cJSON_CreateString(e->name.c_str()));
        cJSON_AddItemToObject(j, "id", cJSON_CreateString(e->id.c_str()));
        return j;
    }

    static cJSON* toJson(OutboundCallParameters *e) {
        auto j = cJSON_CreateObject();
        cJSON_AddItemToObject(j, "video", e->Video ? cJSON_CreateTrue() : cJSON_CreateFalse());
        cJSON_AddItemToObject(j, "screen", e->Screen ? cJSON_CreateTrue() : cJSON_CreateFalse());
        if (!e->AutoAnswer.empty()) {
            cJSON_AddItemToObject(j, "autoAnswer", cJSON_CreateString(e->AutoAnswer.c_str()));
        }
        cJSON_AddItemToObject(j, "disableStun", e->DisableStun ? cJSON_CreateTrue() : cJSON_CreateFalse());
        return j;
    }

    static void setBodyCallInfo(cJSON *j, Info *info) {
        cJSON_AddItemToObject(j, "direction", cJSON_CreateString(info->direction.c_str()));
        cJSON_AddItemToObject(j, "destination", cJSON_CreateString(info->destination.c_str()));
        if (!info->parent_id.empty()) {
            cJSON_AddItemToObject(j, "parent_id", cJSON_CreateString(info->parent_id.c_str()));
        }
        if (info->from) {
            cJSON_AddItemToObject(j, "from", toJson(info->from));
        }
        if (info->to) {
            cJSON_AddItemToObject(j, "to", toJson(info->to));
        }
    }

    inline std::string eavesdropStateName() {
        std::string tmp = event_->getVar("variable_" WBT_EAVESDROP_STATE);
        if (tmp.empty()) {
            tmp = "none";
        }

        return std::move(tmp);
    }

    void setEavesdrop(std::string &type) {
        cJSON *cj;
        cj = cJSON_CreateObject();
        cJSON_AddItemToObject(cj, "type", cJSON_CreateString(type.c_str()));

        auto tmp = event_->getVar("variable_wbt_eavesdrop_name");
        if (!tmp.empty()) {
            cJSON_AddItemToObject(cj, "name", cJSON_CreateString(tmp.c_str()));
        }

        tmp = event_->getVar("variable_wbt_eavesdrop_number");
        if (!tmp.empty()) {
            cJSON_AddItemToObject(cj, "number", cJSON_CreateString(tmp.c_str()));
        }

        tmp = event_->getVar("variable_wbt_eavesdrop_duration");
        if (!tmp.empty()) {
            cJSON_AddItemToObject(cj, "duration", cJSON_CreateNumber(std::stoi(tmp)));
        }

        tmp = eavesdropStateName();
        cJSON_AddItemToObject(cj, "state", cJSON_CreateString(tmp.c_str()));
        addAttribute("eavesdrop", cj);
        /*
         {
          "id": "85b9366a-4c2f-45b0-bcea-06a38fe4be37",
          "control": true,
          "listenA": true,
          "listenB": true
        }
         */
    }

    static void setCallParameters(cJSON *j, OutboundCallParameters *params) {
        cJSON_AddItemToObject(j, "params", toJson(params));
    }

    inline std::string getDestination() {
        std::string res = event_->getVar("Channel-Destination-Number");
        if (!res.empty()) {
            return res;
        }

        res = event_->getVar("Caller-Destination-Number");
        if (!res.empty()) {
            return res;
        }

        res = event_->getVar("variable_destination_number");
        return res;
    }

    OutboundCallParameters getCallParams() {
        auto params = OutboundCallParameters();
        params.Video = event_->getVar("variable_wbt_video") == "true";
        params.Screen = event_->getVar("variable_wbt_screen") == "true";
        params.AutoAnswer = event_->getVar("variable_wbt_auto_answer");
        params.DisableStun = event_->getVar("variable_wbt_disable_stun") == "true";
        return params;
    }

    bool isOriginateRequest() {
        return !event_->getVar("variable_sip_h_X-Webitel-Display-Direction").empty();
    }

    void initContact() {
        if (event_->getVar("variable_wbt_hide_contact") == "true") {
            addAttribute("hide_contact", true);
        }

        auto contact = event_->getVar("variable_wbt_contact_id");
        if (!contact.empty()){
            addAttribute("contact_id", std::stoi( contact ));
        }
    }

    Info getCallInfo() {
        auto info = Info();
        if (parent_) {
            info.parent_id = std::string(parent_);
        }

        info.direction = event_->getVar("variable_sip_h_X-Webitel-Direction");
        auto logicalDirection = event_->getVar("Call-Direction");
        auto isOriginate = isOriginateRequest();

        if (info.direction == "internal" ){
            info.direction = logicalDirection == "outbound" && !isOriginate ? "inbound" : "outbound";
        }

        char *destination = nullptr;
        if (isOriginate) {
            destination = strdup(event_->getVar("variable_effective_callee_id_number").data());
        } else {
            destination = strdup(getDestination().data());
        }
        switch_url_decode(destination);
        info.destination = std::string(destination);
        delete destination;

        auto gateway = event_->getVar("variable_sip_h_X-Webitel-Gateway-Id");
        auto user = event_->getVar("variable_sip_h_X-Webitel-User-Id");
        info.from = new CallEndpoint;

        if (!cc_node_.empty()) {
            info.from->id = event_->getVar("variable_wbt_from_id");
            info.from->number = event_->getVar("variable_wbt_from_number");
            info.from->name = event_->getVar("variable_wbt_from_name");
            info.from->type = event_->getVar("variable_wbt_from_type");

            if (!gateway.empty()) {
                addAttribute(HEADER_NAME_GATEWAY_ID, static_cast<double>(std::stoi(gateway)));
            }

            auto toType = event_->getVar("variable_wbt_to_type");
            if (!toType.empty()) {
                info.to = new CallEndpoint;
                info.to->id = event_->getVar("variable_wbt_to_id");
                info.to->name = event_->getVar("variable_wbt_to_name");
                info.to->number = event_->getVar("variable_wbt_to_number");
                info.to->type = toType;
            }
        } else if ( !gateway.empty() && user.empty()) {
            addAttribute(HEADER_NAME_GATEWAY_ID, static_cast<double>(std::stoi(gateway)));
            if (info.direction == "inbound") {
                info.from->type = "dest";
                info.from->name = event_->getVar("Caller-Caller-ID-Name");
                info.from->number = event_->getVar("Caller-Caller-ID-Number");

//                info.to = new CallEndpoint;
//                info.to->type = "gateway";
//                info.to->id = gateway;
//                info.to->name = event_->getVar("variable_sip_h_X-Webitel-Gateway");
//                info.to->number = event_->getVar("Caller-Caller-ID-Number");
            } else {
                info.from->id = event_->getVar("variable_wbt_from_id");
                info.from->number = event_->getVar("variable_wbt_from_number");
                info.from->name = event_->getVar("variable_wbt_from_name");
                info.from->type = event_->getVar("variable_wbt_from_type");

                auto toType = event_->getVar("variable_wbt_to_type");
                if (!toType.empty()) {
                    info.to = new CallEndpoint;
                    info.to->id = event_->getVar("variable_wbt_to_id");
                    info.to->name = event_->getVar("variable_wbt_to_name");
                    info.to->number = event_->getVar("variable_wbt_to_number");
                    info.to->type = toType;
                }
            }
        } else if (!user.empty()) {
            if (info.direction == "inbound") {
                info.destination = event_->getVar("variable_wbt_destination");
                info.from->type = event_->getVar("variable_wbt_from_type");
                info.from->id = event_->getVar("variable_wbt_from_id");
                info.from->number = event_->getVar("Other-Leg-Caller-ID-Number");
                info.from->name = event_->getVar("Other-Leg-Caller-ID-Name");
                if (info.from->number.empty()) {
                    info.from->number = event_->getVar("variable_wbt_from_number");
                }
                if (info.from->name.empty()) {
                    info.from->name = event_->getVar("variable_wbt_from_name");
                }

                info.to = new CallEndpoint;
                info.to->type = "user";
                info.to->id = user;
                info.to->name = event_->getVar("variable_wbt_to_name");
                info.to->number = event_->getVar("variable_wbt_to_number");
            } else {
                info.from->type = "user";
                info.from->id = user;
                if (isOriginate) {
                    info.from->number = event_->getVar("variable_effective_caller_id_number");
                    info.from->name = event_->getVar("variable_effective_caller_id_name");
                } else {
                    info.from->number = event_->getVar("Caller-Caller-ID-Number");
                    info.from->name = event_->getVar("Caller-Caller-ID-Name");
                }

                auto toType = event_->getVar("variable_wbt_to_type");
                if (!toType.empty()) {
                    info.to = new CallEndpoint;
                    info.to->id = event_->getVar("variable_wbt_to_id");
                    info.to->name = event_->getVar("variable_wbt_to_name");
                    info.to->number = event_->getVar("variable_wbt_to_number");
                    info.to->type = toType;
                }
            }
        } else {
            // TODO from site
            info.from->type = "";
            info.from->id = "";
            info.from->number = event_->getVar("Caller-Caller-ID-Number");
            info.from->name = event_->getVar("Caller-Caller-ID-Name");
        }

        return info;
    }

    void setOnCreateAttr() {
        addIfExists(body_, "sip_id", "variable_sip_h_X-Webitel-Uuid");
        auto grantee = get_str(switch_event_get_header(e_, "variable_wbt_grantee_id"));
        if (!grantee.empty()) {
            addAttribute("grantee_id", std::stoi( grantee ));
        }
    }

    void addIfExists(cJSON *cj, const char *name, const char *varName ) {
        auto tmp = get_str(switch_event_get_header(e_, varName));
        if (!tmp.empty()) {
            cJSON_AddItemToObject(cj, name, cJSON_CreateString(tmp.c_str()));
        }
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

    void setVariables (const char *pref, const char *fieldName, switch_event_t *event) {
        switch_event_header_t *hp;
        cJSON *cj;
        const size_t len = strlen(pref);
        bool found(false);

        cj = cJSON_CreateObject();

        for (hp = event->headers; hp; hp = hp->next) {

            if (!prefix(pref, hp->name) ) {
                continue;
            }
            found = true;

            auto name = std::string(hp->name);
            name = name.substr(len, name.length());

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
            addAttribute(fieldName, cj);
            cj = nullptr;
        } else {
            cJSON_Delete(cj);
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
        setOnCreateAttr();
        if (event_->getVar("variable_wbt_hide_number") == "true") {
            addAttribute("hideNumber", true);
        }
        if (event_->getVar("variable_wbt_originate") == "true") {
            addAttribute("originate", true);
        }

        auto wbt_heartbeat = event_->getVar("variable_wbt_heartbeat");
        if (!wbt_heartbeat.empty()) {
            int sec = 0;
            sscanf( wbt_heartbeat.c_str(), "%d", &sec );
            if (sec) {
                addAttribute("heartbeat", sec);
            }
        }
        auto info = getCallInfo();
        auto eavesdrop = event_->getVar("variable_wbt_eavesdrop_type");
        if (!eavesdrop.empty()) {
            setEavesdrop(eavesdrop);
        }

        setBodyCallInfo(body_, &info);
        initContact();
        //TODO
        delete info.from;
        delete info.to;

        setVariables("variable_usr_", "payload", e_);

        if ( auto t = switch_event_get_header(e, "variable_usr_wbt_ivr_log")) {
            auto j = cJSON_Parse(t);
            if (j) {
                cJSON_AddItemToObject(body_, "ivr", j);
            }
        }


        if (!cc_node_.empty()) {
            setVariables("variable_cc_", "queue", e_);
        }

        if (isOriginateRequest() || !cc_node_.empty()) {
            auto params = getCallParams();
            setCallParameters(body_, &params);
        }
    };

};

template <> class CallEvent<Active> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Active, e) {
        auto eavesdrop = event_->getVar("variable_wbt_eavesdrop_type");
        if (!eavesdrop.empty()) {
            notifyEavesdropPartner("joined");
        }
    };
};

template <> class CallEvent<Bridge> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Bridge, e) {
        auto direction = event_->getVar("variable_sip_h_X-Webitel-Direction");
        auto logicalDirection = event_->getVar("Call-Direction");
        auto signalBond = event_->getVar("variable_signal_bond");
        if (signalBond.empty()) {
            signalBond = event_->getVar("Other-Leg-Unique-ID");
        }

        if (direction == "internal" ){
            direction = logicalDirection == "outbound" ? "inbound" : "outbound";
        }
        //fixme: hold ui!!!

        initContact();
        setVariables("variable_usr_", "payload", e_);

//        DUMP_EVENT(e);

//        auto to = new CallEndpoint;
//        to->number = event_->getVar("Caller-Callee-ID-Number");
//        to->name = event_->getVar("Caller-Callee-ID-Name");
        auto to = new CallEndpoint;
        if (signalBond == uuid_ ) {
            to->number = event_->getVar("Caller-Caller-ID-Number");
            to->name = event_->getVar("Caller-Caller-ID-Name");
        } else {
            to->number = event_->getVar("Caller-Callee-ID-Number");
            to->name = event_->getVar("Caller-Callee-ID-Name");
        }

        if (event_->getVar("variable_wbt_hide_number") == "true") {
            addAttribute("hideNumber", true);
        }

        addAttribute("to", toJson(to));
        //todo
        delete to;
        addAttribute("direction", direction.c_str());

        if (!signalBond.empty()) {
            addAttribute("bridged_id", signalBond);
        }
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
        auto sip_code_ = get_str(switch_event_get_header(e, "variable_proto_specific_hangup_cause"));
        auto cc_reporting_at_ = switch_event_get_header(e, "variable_cc_reporting_at");
        auto hangup_by = get_str(switch_event_get_header(e, "variable_sip_hangup_disposition"));
        auto wbt_transfer_to = get_str(switch_event_get_header(e, "variable_wbt_transfer_to"));
        auto wbt_transfer_from = get_str(switch_event_get_header(e, "variable_wbt_transfer_from"));
        auto wbt_transfer_to_agent = get_str(switch_event_get_header(e, "variable_wbt_transfer_to_agent"));
        auto wbt_transfer_from_attempt = get_str(switch_event_get_header(e, "variable_wbt_transfer_from_attempt"));
        auto wbt_transfer_to_attempt = get_str(switch_event_get_header(e, "variable_wbt_transfer_to_attempt"));
        auto wbt_talk_sec = get_str(switch_event_get_header(e, "variable_wbt_talk_sec"));
        auto wbt_amd = get_str(switch_event_get_header(e, "variable_"  WBT_AMD_AI));
        auto skip_cdr = switch_false(switch_event_get_header(e, "variable_" SKIP_EVENT_VARIABLE));
        auto sip_hangup_phrase = get_str(switch_event_get_header(e, "variable_sip_hangup_phrase"));
        if (sip_hangup_phrase.empty()) {
            sip_hangup_phrase = get_str(switch_event_get_header(e, "variable_sip_invite_failure_phrase"));
        }

        // DEV-5528
        auto variable_last_bridge_to = get_str(switch_event_get_header(e, "variable_last_bridge_to"));
        if (cause_ == "ATTENDED_TRANSFER" && wbt_transfer_from.empty() && !variable_last_bridge_to.empty()) {
            auto session = switch_core_session_locate(variable_last_bridge_to.c_str());
            if (session) {
                wbt_transfer_from = get_str(switch_channel_get_variable(switch_core_session_get_channel(session), "bridge_uuid"));
                switch_core_session_rwunlock(session);
            }
        }

        auto eavesdrop = event_->getVar("variable_wbt_eavesdrop_type");
        if (!eavesdrop.empty()) {
            notifyEavesdropPartner("leave");
        }

//        DUMP_EVENT(e);
        if (skip_cdr) {
            addAttribute("cdr", false);
        }

        if (event_->getVar("variable_wbt_notification_hangup") == "true") {
            addAttribute("notification_hangup", true);
        }

        addIfExists(body_, "amd_result", "variable_amd_result");
        addIfExists(body_, "amd_cause", "variable_amd_cause");
        initContact();

        auto record_seconds = get_str(switch_event_get_header(e, "variable_record_seconds"));
        if (!record_seconds.empty() && record_seconds != "0") {
            auto record_start = get_str(switch_event_get_header(e, "variable_wbt_start_record"));
            if (!record_start.empty()) {
                if (switch_true(switch_event_get_header(e, "variable_media_bug_answer_req"))) {
                    auto br = get_str(switch_event_get_header(e, "variable_bridge_epoch"));
                    if (!br.empty()) {
                        addAttribute("record_start", br + "000");
                    }
                } else {
                    addIfExists(body_, "record_start", "variable_wbt_start_record");
                }
                addIfExists(body_, "record_stop", "variable_wbt_stop_record");
            }
        }

        if (!wbt_transfer_to.empty()) {
            addAttribute("transfer_to", wbt_transfer_to);
        }
        if (!wbt_transfer_from.empty()) {
            addAttribute("transfer_from", wbt_transfer_from);
        }
        if (!wbt_transfer_to_agent.empty()) {
            addAttribute("transfer_to_agent", wbt_transfer_to_agent);
        }
        if (!wbt_transfer_from_attempt.empty()) {
            addAttribute("transfer_from_attempt", wbt_transfer_from_attempt);
        }
        if (!wbt_transfer_to_attempt.empty()) {
            addAttribute("transfer_to_attempt", wbt_transfer_to_attempt);
        }
        if (!wbt_talk_sec.empty()) {
            addAttribute("talk_sec", wbt_talk_sec);
        }

        if (switch_event_get_header(e, "variable_grpc_send_hangup") != nullptr || hangup_by == "recv_bye" ||
            hangup_by == "recv_refuse" || hangup_by == "recv_cancel" || (hangup_by == "send_refuse" && parent_)) {
            addAttribute("hangup_by", parent_ ? "B" : "A");
        } else if ((get_str(switch_event_get_header(e, "variable_last_app"))) == "hangup") {
            addAttribute("hangup_by", "F");
        } else {
            addAttribute("hangup_by", parent_ ? "A" : "B");
        }

//        DUMP_EVENT(e)

        if (cc_reporting_at_) {
            addAttribute("reporting_at", cc_reporting_at_);
        }

        setVariables("variable_usr_", "payload", e_);

        addAttribute(HEADER_NAME_HANGUP_CAUSE, cause_);
        addAttribute("originate_success",
                     switch_event_get_header(e, "variable_grpc_originate_success") != nullptr);

        int num = 0;

        if (!sip_code_.empty()) {
            sscanf( sip_code_.c_str(), "sip:%d", &num );
        } else {
            sip_code_ = get_str(switch_event_get_header(e, "variable_sip_invite_failure_status"));
            if (sip_code_.empty()) {
                sip_code_ = get_str(switch_event_get_header(e, "variable_sip_term_status"));
            }

            if (!sip_code_.empty()) {
                sscanf( sip_code_.c_str(), "%d", &num );
            }
        }

        if (num == 0) {
            addAttribute("sip", cause_ == "ORIGINATOR_CANCEL" ? 487 : 200);
        } else {
            addAttribute("sip", num);
        }

        auto hp = switch_event_get_header_ptr(e, "variable_wbt_tags");
        if (hp) {
            addArrayValue(hp, "tags", false);
        }

        auto sids = switch_event_get_header_ptr(e, "variable_wbt_schema_ids");
        if (sids) {
            addArrayValue(sids, "schema_ids", true);
        }

        if (!wbt_amd.empty()) {
            auto positive = get_str(switch_event_get_header(e, "variable_" WBT_AMD_AI_POSITIVE));
            addAttribute("amd_ai_result", wbt_amd);
            addAttribute( "amd_ai_positive", positive == "true");
            hp = switch_event_get_header_ptr(e, "variable_" WBT_AMD_AI_LOG);
            if (hp) {
                addArrayValue(hp, "amd_ai_logs", false);
            }
        }

        if (!sip_hangup_phrase.empty()) {
            addAttribute("hangup_phrase", sip_hangup_phrase);
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
        set_queue_data(e);
    };
};

template <> class CallEvent<LeavingQueue> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(LeavingQueue, e) {

    };
};

template <> class CallEvent<AMD> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(AMD, e) {
        addIfExists(body_, "ai_result", "variable_"  WBT_AMD_AI);
        addIfExists(body_, "ai_error", "variable_"  WBT_AMD_AI_ERROR);
        addIfExists(body_, "result", "variable_amd_result");
        addIfExists(body_, "cause", "variable_amd_cause");
    };
};

template <> class CallEvent<Eavesdrop> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Eavesdrop, e) {
        auto state = eavesdropStateName();
        addAttribute("state", state);
        addIfExists(body_, "type", "variable_wbt_eavesdrop_type");
        notifyEavesdropPartner(state);
    };
};

template <> class CallEvent<Heartbeat> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Heartbeat, e) {

    };
};

template <> class CallEvent<Transcript> : public BaseCallEvent {
public:
    explicit CallEvent(switch_event_t *e) : BaseCallEvent(Transcript, e) {
        const char* tjson = switch_event_get_body(e);
        if (tjson) {
            cJSON  *json = cJSON_Parse(tjson);
            addAttribute("transcript", json);
        }
    };
};


#endif //MOD_GRPC_CALL_H

//
// Created by root on 24.06.19.
//

#ifndef MOD_GRPC_CALLMANAGER_H
#define MOD_GRPC_CALLMANAGER_H

extern "C" {
#include <switch.h>
#include <switch_event.h>
}

#include "Call.h"

namespace mod_grpc {

    class CallManager {
    public:
        explicit CallManager();
        ~CallManager();

    protected:
        static void handle_call_event(switch_event_t *event);
    };
}


#endif //MOD_GRPC_CALLMANAGER_H

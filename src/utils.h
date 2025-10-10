//
// Created by root on 3/10/22.
//

#ifndef MOD_GRPC_UTILS_H
#define MOD_GRPC_UTILS_H

#include <switch.h>

namespace mod_grpc {
    static long int unixTimestamp() {
        switch_time_t ts = switch_micro_time_now();
        return static_cast<long int>(ts / 1000);
    }
}
#endif //MOD_GRPC_UTILS_H

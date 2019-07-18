//
// Created by igor on 17.07.19.
//

#ifndef MOD_GRPC_CLUSTER_H
#define MOD_GRPC_CLUSTER_H

extern "C" {
#include <switch.h>
}

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "switch_curl.h"

namespace mod_grpc {
    static long sendRequest(const char *uri, const char *body);


    class Timer {
        bool stopped; // This is the thing the thread is waiting for
        std::thread thread;
        std::mutex mutex;
        std::condition_variable terminate;

        void stop();
    public:
        ~Timer();
        template<typename Function>
        void setTimeout(Function function, int interval);
    };

    class Cluster {
        Timer *timer_;

        std::string id_;
        std::string address_;
        int port_;

        std::string register_uri;
        std::string deregister_uri;
        std::string check_uri;

        void ttl();
        void registerService();
        void unregisterService();
    public:
        explicit Cluster(const std::string &server, const std::string &address, const int &port);
        ~Cluster();
    };

}


#endif //MOD_GRPC_CLUSTER_H

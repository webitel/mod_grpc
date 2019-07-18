//
// Created by igor on 17.07.19.
//


#include "Cluster.h"

#define SERVICE_NAME "freeswitch"
#define REGISTER_PATH "/v1/agent/service/register"
#define UN_REGISTER_PATH "/v1/agent/service/deregister/"
#define CHECK_PATH "/v1/agent/check/pass/service:"

namespace mod_grpc {

    template<typename Function>
    void Timer::setTimeout(Function function, int interval) {
        stop();
        {
            auto locked = std::unique_lock<std::mutex>(mutex);
            stopped = false;
        }

        thread = std::thread([=]() {
            auto locked = std::unique_lock<std::mutex>(mutex);

            while (!stopped) // We hold the mutex that protects stop
            {
                auto result = terminate.wait_for(locked, std::chrono::milliseconds(interval));

                if (result == std::cv_status::timeout) {
                    function();
                }
            }
        });
    }

    void Timer::stop() {
        {
            auto locked = std::unique_lock<std::mutex>(mutex);
            stopped = true;
        }

        terminate.notify_one();

        if(thread.joinable()) {
            thread.join();
        }
    }

    Timer::~Timer() {
        stop();
    }


    Cluster::Cluster(const std::string &server, const std::string &address, const int &port, const int &ttl, const int &deregister_ttl) : address_(address), port_(port) {

        timer_ = new Timer();
        id_ = std::string(switch_core_get_switchname());

        register_uri =  server + REGISTER_PATH;
        deregister_uri =  server + UN_REGISTER_PATH;
        check_uri =  server + CHECK_PATH + id_;

        registerService(ttl, deregister_ttl);
    }

    Cluster::~Cluster() {
        delete timer_;
        unregisterService();
    }

    void Cluster::registerService(const int &ttl_s, const int &deregister_ttl) {

        std::string body = R"({"Name" : ")" + std::string(SERVICE_NAME) + R"(", "ID": ")" + id_ + R"(", "Address": ")" + address_ +
                           R"(", "Port": )" + std::to_string(port_) + ","
                           "\"Check\": {\"DeregisterCriticalServiceAfter\": \"" + std::to_string(deregister_ttl) +
                           R"(s","TTL": ")" + std::to_string(ttl_s) + "s\"}" + "}";

        if (sendRequest(register_uri.c_str(), body.c_str()) != 200) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cluster] error register\n");
            throw -1;
        };
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[cluster]  registered: %s\n", body.c_str());
        ttl();
        timer_->setTimeout(std::bind(&Cluster::ttl, this), (ttl_s * 1000) / 2);
    }

    void Cluster::unregisterService() {
        sendRequest((deregister_uri + id_).c_str(), nullptr);
    }

    void Cluster::ttl() {
        if (sendRequest(check_uri.c_str(), nullptr) != 200) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cluster] send ttl error\n");
        };
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[cluster] send ttl pass\n");
    }

    static long sendRequest(const char *uri, const char *body) {
        switch_CURL  *cli = switch_curl_easy_init();
        long response_code = -1;

        if (cli) {
            switch_CURLcode res;
            switch_curl_slist_t *headers = nullptr;
            switch_curl_easy_setopt(cli, CURLOPT_URL,  uri);
            headers = switch_curl_slist_append(headers, "Content-Type: application/json");
            switch_curl_easy_setopt(cli, CURLOPT_HTTPHEADER, headers);

#ifdef DEBUG_CURL
            switch_curl_easy_setopt(cli, CURLOPT_VERBOSE, 1L);
#endif
            switch_curl_easy_setopt(cli, CURLOPT_CUSTOMREQUEST, "PUT");

            if (body) {
                switch_curl_easy_setopt(cli, CURLOPT_POSTFIELDS, body);
            }

            res = switch_curl_easy_perform(cli);
            if(res == CURLE_OK) {
                curl_easy_getinfo(cli, CURLINFO_RESPONSE_CODE, &response_code);
            }

            switch_curl_easy_cleanup(cli);
            switch_curl_slist_free_all(headers);
        }

        return response_code;
    }

}
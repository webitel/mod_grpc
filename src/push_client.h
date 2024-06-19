//
// Created by root on 18.06.24.
//

#ifndef MOD_GRPC_PUSH_CLIENT_H
#define MOD_GRPC_PUSH_CLIENT_H

extern "C" {
#include <switch.h>
}

#include <iostream>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#include "generated/push.pb.h"
#include "generated/push.grpc.pb.h"


namespace mod_grpc {

    struct PushData {
        std::string call_id;
        std::string from_number;
        std::string from_name;
        std::string direction;
        int auto_answer;
        int delay;
    };

    class PushClient {
    public:
        PushClient(const std::string& endpoint) {
            this->client = ::engine::PushService::NewStub(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()));

        }

        int Send(std::vector<std::string> &android, std::vector<std::string> &apple, switch_core_session_t *session, int ttl, PushData *d) {
            grpc::ClientContext context;
            ::engine::SendPushResponse res;
            ::engine::SendPushRequest req;
            bool ok = false;

            if (!android.empty()) {
                *req.mutable_android() = {android.begin(), android.end()};
                ok = true;
            }
            if (!apple.empty()) {
                *req.mutable_apple() = {apple.begin(), apple.end()};
                ok = true;
            }

            if (!ok) {
                return 0;
            }

            // Set timeout for API
            std::chrono::system_clock::time_point deadline =
                    std::chrono::system_clock::now() + std::chrono::seconds(20);

            context.set_deadline(deadline);

            req.set_priority(10);
            req.set_expiration(ttl);

            auto data = req.mutable_data();
            data->insert({"type", "call"});
            if (!d->call_id.empty()) {
                data->insert({"call_id", d->call_id});
            }
            if (!d->from_number.empty()) {
                data->insert({"from_number", d->from_number});
            }
            if (!d->from_name.empty()) {
                data->insert({"from_name", d->from_name});
            }
            if (!d->direction.empty()) {
                data->insert({"direction", d->direction});
            }
            data->insert({"auto_answer", std::to_string(d->auto_answer)});

            auto status = this->client->SendPush(&context, req, &res);
            if (!status.ok()) {
                switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR, "push request error: %s [%d]\n",
                                  status.error_message().c_str(), status.error_code());
            }

            return res.send();
        }

        ~PushClient() {
            this->client.reset();
        }
    private:
        std::unique_ptr<::engine::PushService::Stub>  client;
    };
}


#endif //MOD_GRPC_PUSH_CLIENT_H

// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: push.proto
#ifndef GRPC_push_2eproto__INCLUDED
#define GRPC_push_2eproto__INCLUDED

#include "push.pb.h"

#include <functional>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/proto_utils.h>
#include <grpcpp/impl/rpc_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
#include <grpcpp/support/sync_stream.h>

namespace engine {

class PushService final {
 public:
  static constexpr char const* service_full_name() {
    return "engine.PushService";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status SendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::engine::SendPushResponse* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::engine::SendPushResponse>> AsyncSendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::engine::SendPushResponse>>(AsyncSendPushRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::engine::SendPushResponse>> PrepareAsyncSendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::engine::SendPushResponse>>(PrepareAsyncSendPushRaw(context, request, cq));
    }
    class async_interface {
     public:
      virtual ~async_interface() {}
      virtual void SendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest* request, ::engine::SendPushResponse* response, std::function<void(::grpc::Status)>) = 0;
      virtual void SendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest* request, ::engine::SendPushResponse* response, ::grpc::ClientUnaryReactor* reactor) = 0;
    };
    typedef class async_interface experimental_async_interface;
    virtual class async_interface* async() { return nullptr; }
    class async_interface* experimental_async() { return async(); }
   private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::engine::SendPushResponse>* AsyncSendPushRaw(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::engine::SendPushResponse>* PrepareAsyncSendPushRaw(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());
    ::grpc::Status SendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::engine::SendPushResponse* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::engine::SendPushResponse>> AsyncSendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::engine::SendPushResponse>>(AsyncSendPushRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::engine::SendPushResponse>> PrepareAsyncSendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::engine::SendPushResponse>>(PrepareAsyncSendPushRaw(context, request, cq));
    }
    class async final :
      public StubInterface::async_interface {
     public:
      void SendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest* request, ::engine::SendPushResponse* response, std::function<void(::grpc::Status)>) override;
      void SendPush(::grpc::ClientContext* context, const ::engine::SendPushRequest* request, ::engine::SendPushResponse* response, ::grpc::ClientUnaryReactor* reactor) override;
     private:
      friend class Stub;
      explicit async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class async* async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::engine::SendPushResponse>* AsyncSendPushRaw(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::engine::SendPushResponse>* PrepareAsyncSendPushRaw(::grpc::ClientContext* context, const ::engine::SendPushRequest& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_SendPush_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status SendPush(::grpc::ServerContext* context, const ::engine::SendPushRequest* request, ::engine::SendPushResponse* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_SendPush : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_SendPush() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_SendPush() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendPush(::grpc::ServerContext* /*context*/, const ::engine::SendPushRequest* /*request*/, ::engine::SendPushResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSendPush(::grpc::ServerContext* context, ::engine::SendPushRequest* request, ::grpc::ServerAsyncResponseWriter< ::engine::SendPushResponse>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_SendPush<Service > AsyncService;
  template <class BaseClass>
  class WithCallbackMethod_SendPush : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_SendPush() {
      ::grpc::Service::MarkMethodCallback(0,
          new ::grpc::internal::CallbackUnaryHandler< ::engine::SendPushRequest, ::engine::SendPushResponse>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::engine::SendPushRequest* request, ::engine::SendPushResponse* response) { return this->SendPush(context, request, response); }));}
    void SetMessageAllocatorFor_SendPush(
        ::grpc::MessageAllocator< ::engine::SendPushRequest, ::engine::SendPushResponse>* allocator) {
      ::grpc::internal::MethodHandler* const handler = ::grpc::Service::GetHandler(0);
      static_cast<::grpc::internal::CallbackUnaryHandler< ::engine::SendPushRequest, ::engine::SendPushResponse>*>(handler)
              ->SetMessageAllocator(allocator);
    }
    ~WithCallbackMethod_SendPush() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendPush(::grpc::ServerContext* /*context*/, const ::engine::SendPushRequest* /*request*/, ::engine::SendPushResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* SendPush(
      ::grpc::CallbackServerContext* /*context*/, const ::engine::SendPushRequest* /*request*/, ::engine::SendPushResponse* /*response*/)  { return nullptr; }
  };
  typedef WithCallbackMethod_SendPush<Service > CallbackService;
  typedef CallbackService ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_SendPush : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_SendPush() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_SendPush() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendPush(::grpc::ServerContext* /*context*/, const ::engine::SendPushRequest* /*request*/, ::engine::SendPushResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_SendPush : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_SendPush() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_SendPush() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendPush(::grpc::ServerContext* /*context*/, const ::engine::SendPushRequest* /*request*/, ::engine::SendPushResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSendPush(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_SendPush : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_SendPush() {
      ::grpc::Service::MarkMethodRawCallback(0,
          new ::grpc::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response) { return this->SendPush(context, request, response); }));
    }
    ~WithRawCallbackMethod_SendPush() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status SendPush(::grpc::ServerContext* /*context*/, const ::engine::SendPushRequest* /*request*/, ::engine::SendPushResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerUnaryReactor* SendPush(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/, ::grpc::ByteBuffer* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_SendPush : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithStreamedUnaryMethod_SendPush() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler<
          ::engine::SendPushRequest, ::engine::SendPushResponse>(
            [this](::grpc::ServerContext* context,
                   ::grpc::ServerUnaryStreamer<
                     ::engine::SendPushRequest, ::engine::SendPushResponse>* streamer) {
                       return this->StreamedSendPush(context,
                         streamer);
                  }));
    }
    ~WithStreamedUnaryMethod_SendPush() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status SendPush(::grpc::ServerContext* /*context*/, const ::engine::SendPushRequest* /*request*/, ::engine::SendPushResponse* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedSendPush(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::engine::SendPushRequest,::engine::SendPushResponse>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_SendPush<Service > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_SendPush<Service > StreamedService;
};

}  // namespace engine


#endif  // GRPC_push_2eproto__INCLUDED

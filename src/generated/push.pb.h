// Generated by the protocol buffer compiler.  DO NOT EDIT!
// NO CHECKED-IN PROTOBUF GENCODE
// source: push.proto
// Protobuf C++ Version: 5.29.0

#ifndef push_2eproto_2epb_2eh
#define push_2eproto_2epb_2eh

#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/runtime_version.h"
#if PROTOBUF_VERSION != 5029000
#error "Protobuf C++ gencode is built with an incompatible version of"
#error "Protobuf C++ headers/runtime. See"
#error "https://protobuf.dev/support/cross-version-runtime-guarantee/#cpp"
#endif
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_field.h"  // IWYU pragma: export
#include "google/protobuf/extension_set.h"  // IWYU pragma: export
#include "google/protobuf/map.h"  // IWYU pragma: export
#include "google/protobuf/map_entry.h"
#include "google/protobuf/map_field_inl.h"
#include "google/protobuf/unknown_field_set.h"
// @@protoc_insertion_point(includes)

// Must be included last.
#include "google/protobuf/port_def.inc"

#define PROTOBUF_INTERNAL_EXPORT_push_2eproto

namespace google {
namespace protobuf {
namespace internal {
template <typename T>
::absl::string_view GetAnyMessageName();
}  // namespace internal
}  // namespace protobuf
}  // namespace google

// Internal implementation detail -- do not use these members.
struct TableStruct_push_2eproto {
  static const ::uint32_t offsets[];
};
extern const ::google::protobuf::internal::DescriptorTable
    descriptor_table_push_2eproto;
namespace engine {
class SendPushRequest;
struct SendPushRequestDefaultTypeInternal;
extern SendPushRequestDefaultTypeInternal _SendPushRequest_default_instance_;
class SendPushRequest_DataEntry_DoNotUse;
struct SendPushRequest_DataEntry_DoNotUseDefaultTypeInternal;
extern SendPushRequest_DataEntry_DoNotUseDefaultTypeInternal _SendPushRequest_DataEntry_DoNotUse_default_instance_;
class SendPushResponse;
struct SendPushResponseDefaultTypeInternal;
extern SendPushResponseDefaultTypeInternal _SendPushResponse_default_instance_;
}  // namespace engine
namespace google {
namespace protobuf {
}  // namespace protobuf
}  // namespace google

namespace engine {

// ===================================================================


// -------------------------------------------------------------------

class SendPushResponse final
    : public ::google::protobuf::Message
/* @@protoc_insertion_point(class_definition:engine.SendPushResponse) */ {
 public:
  inline SendPushResponse() : SendPushResponse(nullptr) {}
  ~SendPushResponse() PROTOBUF_FINAL;

#if defined(PROTOBUF_CUSTOM_VTABLE)
  void operator delete(SendPushResponse* msg, std::destroying_delete_t) {
    SharedDtor(*msg);
    ::google::protobuf::internal::SizedDelete(msg, sizeof(SendPushResponse));
  }
#endif

  template <typename = void>
  explicit PROTOBUF_CONSTEXPR SendPushResponse(
      ::google::protobuf::internal::ConstantInitialized);

  inline SendPushResponse(const SendPushResponse& from) : SendPushResponse(nullptr, from) {}
  inline SendPushResponse(SendPushResponse&& from) noexcept
      : SendPushResponse(nullptr, std::move(from)) {}
  inline SendPushResponse& operator=(const SendPushResponse& from) {
    CopyFrom(from);
    return *this;
  }
  inline SendPushResponse& operator=(SendPushResponse&& from) noexcept {
    if (this == &from) return *this;
    if (::google::protobuf::internal::CanMoveWithInternalSwap(GetArena(), from.GetArena())) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }

  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const SendPushResponse& default_instance() {
    return *internal_default_instance();
  }
  static inline const SendPushResponse* internal_default_instance() {
    return reinterpret_cast<const SendPushResponse*>(
        &_SendPushResponse_default_instance_);
  }
  static constexpr int kIndexInFileMessages = 2;
  friend void swap(SendPushResponse& a, SendPushResponse& b) { a.Swap(&b); }
  inline void Swap(SendPushResponse* other) {
    if (other == this) return;
    if (::google::protobuf::internal::CanUseInternalSwap(GetArena(), other->GetArena())) {
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(SendPushResponse* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  SendPushResponse* New(::google::protobuf::Arena* arena = nullptr) const {
    return ::google::protobuf::Message::DefaultConstruct<SendPushResponse>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const SendPushResponse& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom(const SendPushResponse& from) { SendPushResponse::MergeImpl(*this, from); }

  private:
  static void MergeImpl(
      ::google::protobuf::MessageLite& to_msg,
      const ::google::protobuf::MessageLite& from_msg);

  public:
  bool IsInitialized() const {
    return true;
  }
  ABSL_ATTRIBUTE_REINITIALIZES void Clear() PROTOBUF_FINAL;
  #if defined(PROTOBUF_CUSTOM_VTABLE)
  private:
  static ::size_t ByteSizeLong(const ::google::protobuf::MessageLite& msg);
  static ::uint8_t* _InternalSerialize(
      const MessageLite& msg, ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream);

  public:
  ::size_t ByteSizeLong() const { return ByteSizeLong(*this); }
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream) const {
    return _InternalSerialize(*this, target, stream);
  }
  #else   // PROTOBUF_CUSTOM_VTABLE
  ::size_t ByteSizeLong() const final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  #endif  // PROTOBUF_CUSTOM_VTABLE
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::google::protobuf::Arena* arena);
  static void SharedDtor(MessageLite& self);
  void InternalSwap(SendPushResponse* other);
 private:
  template <typename T>
  friend ::absl::string_view(
      ::google::protobuf::internal::GetAnyMessageName)();
  static ::absl::string_view FullMessageName() { return "engine.SendPushResponse"; }

 protected:
  explicit SendPushResponse(::google::protobuf::Arena* arena);
  SendPushResponse(::google::protobuf::Arena* arena, const SendPushResponse& from);
  SendPushResponse(::google::protobuf::Arena* arena, SendPushResponse&& from) noexcept
      : SendPushResponse(arena) {
    *this = ::std::move(from);
  }
  const ::google::protobuf::internal::ClassData* GetClassData() const PROTOBUF_FINAL;
  static void* PlacementNew_(const void*, void* mem,
                             ::google::protobuf::Arena* arena);
  static constexpr auto InternalNewImpl_();
  static const ::google::protobuf::internal::ClassDataFull _class_data_;

 public:
  ::google::protobuf::Metadata GetMetadata() const;
  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------
  enum : int {
    kSendFieldNumber = 1,
  };
  // int32 send = 1;
  void clear_send() ;
  ::int32_t send() const;
  void set_send(::int32_t value);

  private:
  ::int32_t _internal_send() const;
  void _internal_set_send(::int32_t value);

  public:
  // @@protoc_insertion_point(class_scope:engine.SendPushResponse)
 private:
  class _Internal;
  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      0, 1, 0,
      0, 2>
      _table_;

  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {
    inline explicit constexpr Impl_(
        ::google::protobuf::internal::ConstantInitialized) noexcept;
    inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                          ::google::protobuf::Arena* arena);
    inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                          ::google::protobuf::Arena* arena, const Impl_& from,
                          const SendPushResponse& from_msg);
    ::int32_t send_;
    ::google::protobuf::internal::CachedSize _cached_size_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_push_2eproto;
};
// -------------------------------------------------------------------

class SendPushRequest_DataEntry_DoNotUse final
    : public ::google::protobuf::internal::MapEntry<
          std::string, std::string,
          ::google::protobuf::internal::WireFormatLite::TYPE_STRING,
          ::google::protobuf::internal::WireFormatLite::TYPE_STRING> {
 public:
  using SuperType = ::google::protobuf::internal::MapEntry<
      std::string, std::string,
      ::google::protobuf::internal::WireFormatLite::TYPE_STRING,
      ::google::protobuf::internal::WireFormatLite::TYPE_STRING>;
  SendPushRequest_DataEntry_DoNotUse();
  template <typename = void>
  explicit PROTOBUF_CONSTEXPR SendPushRequest_DataEntry_DoNotUse(
      ::google::protobuf::internal::ConstantInitialized);
  explicit SendPushRequest_DataEntry_DoNotUse(::google::protobuf::Arena* arena);
  static const SendPushRequest_DataEntry_DoNotUse* internal_default_instance() {
    return reinterpret_cast<const SendPushRequest_DataEntry_DoNotUse*>(
        &_SendPushRequest_DataEntry_DoNotUse_default_instance_);
  }


 private:
  friend class ::google::protobuf::MessageLite;
  friend struct ::TableStruct_push_2eproto;

  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      1, 2, 0,
      49, 2>
      _table_;

  const ::google::protobuf::internal::ClassData* GetClassData() const PROTOBUF_FINAL;
  static void* PlacementNew_(const void*, void* mem,
                             ::google::protobuf::Arena* arena);
  static constexpr auto InternalNewImpl_();
  static const ::google::protobuf::internal::ClassDataFull _class_data_;
};
// -------------------------------------------------------------------

class SendPushRequest final
    : public ::google::protobuf::Message
/* @@protoc_insertion_point(class_definition:engine.SendPushRequest) */ {
 public:
  inline SendPushRequest() : SendPushRequest(nullptr) {}
  ~SendPushRequest() PROTOBUF_FINAL;

#if defined(PROTOBUF_CUSTOM_VTABLE)
  void operator delete(SendPushRequest* msg, std::destroying_delete_t) {
    SharedDtor(*msg);
    ::google::protobuf::internal::SizedDelete(msg, sizeof(SendPushRequest));
  }
#endif

  template <typename = void>
  explicit PROTOBUF_CONSTEXPR SendPushRequest(
      ::google::protobuf::internal::ConstantInitialized);

  inline SendPushRequest(const SendPushRequest& from) : SendPushRequest(nullptr, from) {}
  inline SendPushRequest(SendPushRequest&& from) noexcept
      : SendPushRequest(nullptr, std::move(from)) {}
  inline SendPushRequest& operator=(const SendPushRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline SendPushRequest& operator=(SendPushRequest&& from) noexcept {
    if (this == &from) return *this;
    if (::google::protobuf::internal::CanMoveWithInternalSwap(GetArena(), from.GetArena())) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }

  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const SendPushRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const SendPushRequest* internal_default_instance() {
    return reinterpret_cast<const SendPushRequest*>(
        &_SendPushRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages = 1;
  friend void swap(SendPushRequest& a, SendPushRequest& b) { a.Swap(&b); }
  inline void Swap(SendPushRequest* other) {
    if (other == this) return;
    if (::google::protobuf::internal::CanUseInternalSwap(GetArena(), other->GetArena())) {
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(SendPushRequest* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  SendPushRequest* New(::google::protobuf::Arena* arena = nullptr) const {
    return ::google::protobuf::Message::DefaultConstruct<SendPushRequest>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const SendPushRequest& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom(const SendPushRequest& from) { SendPushRequest::MergeImpl(*this, from); }

  private:
  static void MergeImpl(
      ::google::protobuf::MessageLite& to_msg,
      const ::google::protobuf::MessageLite& from_msg);

  public:
  bool IsInitialized() const {
    return true;
  }
  ABSL_ATTRIBUTE_REINITIALIZES void Clear() PROTOBUF_FINAL;
  #if defined(PROTOBUF_CUSTOM_VTABLE)
  private:
  static ::size_t ByteSizeLong(const ::google::protobuf::MessageLite& msg);
  static ::uint8_t* _InternalSerialize(
      const MessageLite& msg, ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream);

  public:
  ::size_t ByteSizeLong() const { return ByteSizeLong(*this); }
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream) const {
    return _InternalSerialize(*this, target, stream);
  }
  #else   // PROTOBUF_CUSTOM_VTABLE
  ::size_t ByteSizeLong() const final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  #endif  // PROTOBUF_CUSTOM_VTABLE
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::google::protobuf::Arena* arena);
  static void SharedDtor(MessageLite& self);
  void InternalSwap(SendPushRequest* other);
 private:
  template <typename T>
  friend ::absl::string_view(
      ::google::protobuf::internal::GetAnyMessageName)();
  static ::absl::string_view FullMessageName() { return "engine.SendPushRequest"; }

 protected:
  explicit SendPushRequest(::google::protobuf::Arena* arena);
  SendPushRequest(::google::protobuf::Arena* arena, const SendPushRequest& from);
  SendPushRequest(::google::protobuf::Arena* arena, SendPushRequest&& from) noexcept
      : SendPushRequest(arena) {
    *this = ::std::move(from);
  }
  const ::google::protobuf::internal::ClassData* GetClassData() const PROTOBUF_FINAL;
  static void* PlacementNew_(const void*, void* mem,
                             ::google::protobuf::Arena* arena);
  static constexpr auto InternalNewImpl_();
  static const ::google::protobuf::internal::ClassDataFull _class_data_;

 public:
  ::google::protobuf::Metadata GetMetadata() const;
  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------
  enum : int {
    kAndroidFieldNumber = 1,
    kAppleFieldNumber = 2,
    kDataFieldNumber = 5,
    kExpirationFieldNumber = 3,
    kPriorityFieldNumber = 4,
  };
  // repeated string android = 1;
  int android_size() const;
  private:
  int _internal_android_size() const;

  public:
  void clear_android() ;
  const std::string& android(int index) const;
  std::string* mutable_android(int index);
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_android(int index, Arg_&& value, Args_... args);
  std::string* add_android();
  template <typename Arg_ = const std::string&, typename... Args_>
  void add_android(Arg_&& value, Args_... args);
  const ::google::protobuf::RepeatedPtrField<std::string>& android() const;
  ::google::protobuf::RepeatedPtrField<std::string>* mutable_android();

  private:
  const ::google::protobuf::RepeatedPtrField<std::string>& _internal_android() const;
  ::google::protobuf::RepeatedPtrField<std::string>* _internal_mutable_android();

  public:
  // repeated string apple = 2;
  int apple_size() const;
  private:
  int _internal_apple_size() const;

  public:
  void clear_apple() ;
  const std::string& apple(int index) const;
  std::string* mutable_apple(int index);
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_apple(int index, Arg_&& value, Args_... args);
  std::string* add_apple();
  template <typename Arg_ = const std::string&, typename... Args_>
  void add_apple(Arg_&& value, Args_... args);
  const ::google::protobuf::RepeatedPtrField<std::string>& apple() const;
  ::google::protobuf::RepeatedPtrField<std::string>* mutable_apple();

  private:
  const ::google::protobuf::RepeatedPtrField<std::string>& _internal_apple() const;
  ::google::protobuf::RepeatedPtrField<std::string>* _internal_mutable_apple();

  public:
  // map<string, string> data = 5;
  int data_size() const;
  private:
  int _internal_data_size() const;

  public:
  void clear_data() ;
  const ::google::protobuf::Map<std::string, std::string>& data() const;
  ::google::protobuf::Map<std::string, std::string>* mutable_data();

  private:
  const ::google::protobuf::Map<std::string, std::string>& _internal_data() const;
  ::google::protobuf::Map<std::string, std::string>* _internal_mutable_data();

  public:
  // int64 expiration = 3;
  void clear_expiration() ;
  ::int64_t expiration() const;
  void set_expiration(::int64_t value);

  private:
  ::int64_t _internal_expiration() const;
  void _internal_set_expiration(::int64_t value);

  public:
  // int32 priority = 4;
  void clear_priority() ;
  ::int32_t priority() const;
  void set_priority(::int32_t value);

  private:
  ::int32_t _internal_priority() const;
  void _internal_set_priority(::int32_t value);

  public:
  // @@protoc_insertion_point(class_scope:engine.SendPushRequest)
 private:
  class _Internal;
  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      2, 5, 1,
      47, 2>
      _table_;

  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {
    inline explicit constexpr Impl_(
        ::google::protobuf::internal::ConstantInitialized) noexcept;
    inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                          ::google::protobuf::Arena* arena);
    inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                          ::google::protobuf::Arena* arena, const Impl_& from,
                          const SendPushRequest& from_msg);
    ::google::protobuf::RepeatedPtrField<std::string> android_;
    ::google::protobuf::RepeatedPtrField<std::string> apple_;
    ::google::protobuf::internal::MapField<SendPushRequest_DataEntry_DoNotUse, std::string, std::string,
                      ::google::protobuf::internal::WireFormatLite::TYPE_STRING,
                      ::google::protobuf::internal::WireFormatLite::TYPE_STRING>
        data_;
    ::int64_t expiration_;
    ::int32_t priority_;
    ::google::protobuf::internal::CachedSize _cached_size_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_push_2eproto;
};

// ===================================================================




// ===================================================================


#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// -------------------------------------------------------------------

// -------------------------------------------------------------------

// SendPushRequest

// repeated string android = 1;
inline int SendPushRequest::_internal_android_size() const {
  return _internal_android().size();
}
inline int SendPushRequest::android_size() const {
  return _internal_android_size();
}
inline void SendPushRequest::clear_android() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.android_.Clear();
}
inline std::string* SendPushRequest::add_android() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  std::string* _s = _internal_mutable_android()->Add();
  // @@protoc_insertion_point(field_add_mutable:engine.SendPushRequest.android)
  return _s;
}
inline const std::string& SendPushRequest::android(int index) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:engine.SendPushRequest.android)
  return _internal_android().Get(index);
}
inline std::string* SendPushRequest::mutable_android(int index)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable:engine.SendPushRequest.android)
  return _internal_mutable_android()->Mutable(index);
}
template <typename Arg_, typename... Args_>
inline void SendPushRequest::set_android(int index, Arg_&& value, Args_... args) {
  ::google::protobuf::internal::AssignToString(
      *_internal_mutable_android()->Mutable(index),
      std::forward<Arg_>(value), args... );
  // @@protoc_insertion_point(field_set:engine.SendPushRequest.android)
}
template <typename Arg_, typename... Args_>
inline void SendPushRequest::add_android(Arg_&& value, Args_... args) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  ::google::protobuf::internal::AddToRepeatedPtrField(*_internal_mutable_android(),
                               std::forward<Arg_>(value),
                               args... );
  // @@protoc_insertion_point(field_add:engine.SendPushRequest.android)
}
inline const ::google::protobuf::RepeatedPtrField<std::string>&
SendPushRequest::android() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_list:engine.SendPushRequest.android)
  return _internal_android();
}
inline ::google::protobuf::RepeatedPtrField<std::string>*
SendPushRequest::mutable_android() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable_list:engine.SendPushRequest.android)
  ::google::protobuf::internal::TSanWrite(&_impl_);
  return _internal_mutable_android();
}
inline const ::google::protobuf::RepeatedPtrField<std::string>&
SendPushRequest::_internal_android() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.android_;
}
inline ::google::protobuf::RepeatedPtrField<std::string>*
SendPushRequest::_internal_mutable_android() {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return &_impl_.android_;
}

// repeated string apple = 2;
inline int SendPushRequest::_internal_apple_size() const {
  return _internal_apple().size();
}
inline int SendPushRequest::apple_size() const {
  return _internal_apple_size();
}
inline void SendPushRequest::clear_apple() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.apple_.Clear();
}
inline std::string* SendPushRequest::add_apple() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  std::string* _s = _internal_mutable_apple()->Add();
  // @@protoc_insertion_point(field_add_mutable:engine.SendPushRequest.apple)
  return _s;
}
inline const std::string& SendPushRequest::apple(int index) const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:engine.SendPushRequest.apple)
  return _internal_apple().Get(index);
}
inline std::string* SendPushRequest::mutable_apple(int index)
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable:engine.SendPushRequest.apple)
  return _internal_mutable_apple()->Mutable(index);
}
template <typename Arg_, typename... Args_>
inline void SendPushRequest::set_apple(int index, Arg_&& value, Args_... args) {
  ::google::protobuf::internal::AssignToString(
      *_internal_mutable_apple()->Mutable(index),
      std::forward<Arg_>(value), args... );
  // @@protoc_insertion_point(field_set:engine.SendPushRequest.apple)
}
template <typename Arg_, typename... Args_>
inline void SendPushRequest::add_apple(Arg_&& value, Args_... args) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  ::google::protobuf::internal::AddToRepeatedPtrField(*_internal_mutable_apple(),
                               std::forward<Arg_>(value),
                               args... );
  // @@protoc_insertion_point(field_add:engine.SendPushRequest.apple)
}
inline const ::google::protobuf::RepeatedPtrField<std::string>&
SendPushRequest::apple() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_list:engine.SendPushRequest.apple)
  return _internal_apple();
}
inline ::google::protobuf::RepeatedPtrField<std::string>*
SendPushRequest::mutable_apple() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable_list:engine.SendPushRequest.apple)
  ::google::protobuf::internal::TSanWrite(&_impl_);
  return _internal_mutable_apple();
}
inline const ::google::protobuf::RepeatedPtrField<std::string>&
SendPushRequest::_internal_apple() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.apple_;
}
inline ::google::protobuf::RepeatedPtrField<std::string>*
SendPushRequest::_internal_mutable_apple() {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return &_impl_.apple_;
}

// int64 expiration = 3;
inline void SendPushRequest::clear_expiration() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.expiration_ = ::int64_t{0};
}
inline ::int64_t SendPushRequest::expiration() const {
  // @@protoc_insertion_point(field_get:engine.SendPushRequest.expiration)
  return _internal_expiration();
}
inline void SendPushRequest::set_expiration(::int64_t value) {
  _internal_set_expiration(value);
  // @@protoc_insertion_point(field_set:engine.SendPushRequest.expiration)
}
inline ::int64_t SendPushRequest::_internal_expiration() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.expiration_;
}
inline void SendPushRequest::_internal_set_expiration(::int64_t value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.expiration_ = value;
}

// int32 priority = 4;
inline void SendPushRequest::clear_priority() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.priority_ = 0;
}
inline ::int32_t SendPushRequest::priority() const {
  // @@protoc_insertion_point(field_get:engine.SendPushRequest.priority)
  return _internal_priority();
}
inline void SendPushRequest::set_priority(::int32_t value) {
  _internal_set_priority(value);
  // @@protoc_insertion_point(field_set:engine.SendPushRequest.priority)
}
inline ::int32_t SendPushRequest::_internal_priority() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.priority_;
}
inline void SendPushRequest::_internal_set_priority(::int32_t value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.priority_ = value;
}

// map<string, string> data = 5;
inline int SendPushRequest::_internal_data_size() const {
  return _internal_data().size();
}
inline int SendPushRequest::data_size() const {
  return _internal_data_size();
}
inline void SendPushRequest::clear_data() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.data_.Clear();
}
inline const ::google::protobuf::Map<std::string, std::string>& SendPushRequest::_internal_data() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.data_.GetMap();
}
inline const ::google::protobuf::Map<std::string, std::string>& SendPushRequest::data() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_map:engine.SendPushRequest.data)
  return _internal_data();
}
inline ::google::protobuf::Map<std::string, std::string>* SendPushRequest::_internal_mutable_data() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  return _impl_.data_.MutableMap();
}
inline ::google::protobuf::Map<std::string, std::string>* SendPushRequest::mutable_data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_mutable_map:engine.SendPushRequest.data)
  return _internal_mutable_data();
}

// -------------------------------------------------------------------

// SendPushResponse

// int32 send = 1;
inline void SendPushResponse::clear_send() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.send_ = 0;
}
inline ::int32_t SendPushResponse::send() const {
  // @@protoc_insertion_point(field_get:engine.SendPushResponse.send)
  return _internal_send();
}
inline void SendPushResponse::set_send(::int32_t value) {
  _internal_set_send(value);
  // @@protoc_insertion_point(field_set:engine.SendPushResponse.send)
}
inline ::int32_t SendPushResponse::_internal_send() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.send_;
}
inline void SendPushResponse::_internal_set_send(::int32_t value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.send_ = value;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)
}  // namespace engine


// @@protoc_insertion_point(global_scope)

#include "google/protobuf/port_undef.inc"

#endif  // push_2eproto_2epb_2eh

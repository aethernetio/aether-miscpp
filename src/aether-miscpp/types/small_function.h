/*
 * Copyright 2026 Aethernet Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AETHER_MISCPP_TYPES_SMALL_FUNCTION_H_
#define AETHER_MISCPP_TYPES_SMALL_FUNCTION_H_

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "aether-miscpp/meta/function_signature.h"
#include "aether-miscpp/types/aligned_storage.h"
#include "aether-miscpp/types/method_ptr.h"  // IWYU pragma: exports

namespace ae {
namespace small_function_internal {
enum class Operation : std::uint8_t {
  kMove,
  kDestroy,
};

template <typename TRet, typename... TArgs>
struct VTable {
  TRet (*invoke)(void*, TArgs&&...);
  void (*manage)(void*, void*, Operation);
};

template <typename TRet, typename... TArgs>
struct Invoker {
  // Invoke for callable objects
  template <typename TCallable>
  static TRet InvokeCallable(void* self, TArgs&&... args) {
    if constexpr (std::is_void_v<TRet>) {
      std::invoke(*std::launder(static_cast<TCallable*>(self)),
                  std::forward<TArgs>(args)...);
    } else {
      return std::invoke(*std::launder(static_cast<TCallable*>(self)),
                         std::forward<TArgs>(args)...);
    }
  }

  // Invoke for free functions
  template <typename TFunction>
  static TRet InvokeFreeFunction(void* self, TArgs&&... args) {
    auto method = *std::launder(static_cast<TFunction*>(self));
    return method(std::forward<TArgs>(args)...);
  }
};

template <typename TCallable>
static void Manage(void* a, void* b, Operation op) noexcept {
  switch (op) {
    case Operation::kMove: {
      new (b) TCallable{std::move(*std::launder(static_cast<TCallable*>(a)))};
      break;
    }
    case Operation::kDestroy: {
      if constexpr (!std::is_trivially_destructible_v<TCallable>) {
        std::destroy_at(std::launder(static_cast<TCallable*>(a)));
      }
      break;
    }
  }
}

template <typename TCallable, typename TRet, typename... TArgs>
static constexpr auto VTableForT = VTable<TRet, TArgs...>{
    Invoker<TRet, TArgs...>::template InvokeCallable<TCallable>,
    Manage<TCallable>,
};

template <typename TFunction, typename TRet, typename... TArgs>
static constexpr auto VTableForFreeFunction = VTable<TRet, TArgs...>{
    Invoker<TRet, TArgs...>::template InvokeFreeFunction<TFunction>,
    Manage<TFunction>,
};

template <typename TCallable, typename TSource, typename TRet,
          typename... TArgs>
/**
 * \brief Whether a callable can be stored and invoked by SmallFunction.
 *
 * For non-void results, the invocation result must be implicitly convertible
 * to TRet; explicit-only conversions are rejected. Void results accept and
 * discard any invocation result, including results requiring explicit
 * conversion.
 */
concept CompatibleCallable =
    std::constructible_from<TCallable, TSource> &&
    std::is_nothrow_constructible_v<TCallable, TSource> &&
    std::is_nothrow_move_constructible_v<TCallable> &&
    std::invocable<TCallable&, TArgs&&...> &&
    (std::is_void_v<TRet> ||
     std::is_convertible_v<std::invoke_result_t<TCallable&, TArgs&&...>, TRet>);
}  // namespace small_function_internal

static constexpr std::size_t kDefaultSize = sizeof(void*) * 4;
static constexpr std::size_t kDefaultAlignment = alignof(void*);

template <typename Signature, std::size_t Size = kDefaultSize,
          std::size_t Alignment = kDefaultAlignment>
class SmallFunction;

template <typename TRet, typename... TArgs, std::size_t Size,
          std::size_t Alignment>
class SmallFunction<TRet(TArgs...), Size, Alignment> {
 public:
  using Storage = AlignedStorage<Size, Alignment>;
  using VTable = small_function_internal::VTable<TRet, TArgs...>;
  using Operation = small_function_internal::Operation;

  SmallFunction() = default;

  ~SmallFunction() noexcept {
    if (vtable_ != nullptr) {
      Destroy();
    }
  }

  /**
   * \brief Construction for any callable types except MethodPtr.
   * By callable it means any type with operator() defined, like lambda or any
   * user provided functor.
   */
  template <typename TFunctor>
    requires(!MethodPtrType<std::decay_t<TFunctor>> &&
             !std::same_as<std::decay_t<TFunctor>, SmallFunction> &&
             small_function_internal::CompatibleCallable<
                 std::decay_t<TFunctor>, TFunctor, TRet, TArgs...>)
  SmallFunction(TFunctor&& functor) noexcept  // NOLINT(*explicit*)
      : vtable_{&small_function_internal::VTableForT<std::decay_t<TFunctor>,
                                                     TRet, TArgs...>} {
    using Type = std::decay_t<TFunctor>;
    static_assert(sizeof(Type) <= Size,
                  "SmallFunction target size exceeds storage size");
    static_assert(alignof(Type) <= Alignment,
                  "SmallFunction target alignment exceeds storage alignment");
    new (storage_.data()) Type{std::forward<TFunctor>(functor)};
  }

  /**
   * \brief Construction for MethodPtr types.
   * This distincts from the any callable types constructor just for future
   * development and its own static assert message.
   */
  template <typename MPtr>
    requires(MethodPtrType<std::decay_t<MPtr>> &&
             small_function_internal::CompatibleCallable<std::decay_t<MPtr>,
                                                         MPtr, TRet, TArgs...>)
  SmallFunction(MPtr&& method_ptr) noexcept  // NOLINT(*explicit*)
      : vtable_{&small_function_internal::VTableForT<std::decay_t<MPtr>, TRet,
                                                     TArgs...>} {
    using Type = std::decay_t<MPtr>;
    static_assert(sizeof(Type) <= Size,
                  "SmallFunction MethodPtr target size exceeds storage size");
    static_assert(
        alignof(Type) <= Alignment,
        "SmallFunction MethodPtr target alignment exceeds storage alignment");
    new (storage_.data()) Type{std::forward<MPtr>(method_ptr)};
  }

  /**
   * \brief Construction for free function
   * It accepts any function pointer type and stores it in the storage.
   */
  SmallFunction(TRet (*func_ptr)(TArgs...)) noexcept  // NOLINT(*explicit*)
      : vtable_{
            &small_function_internal::VTableForFreeFunction<TRet (*)(TArgs...),
                                                            TRet, TArgs...>} {
    using FuncType = decltype(func_ptr);
    static_assert(sizeof(FuncType) <= Size,
                  "SmallFunction function pointer size exceeds storage size");
    static_assert(
        alignof(FuncType) <= Alignment,
        "SmallFunction function pointer alignment exceeds storage alignment");
    new (storage_.data()) FuncType{func_ptr};
  }

  SmallFunction(SmallFunction const& other) = delete;

  SmallFunction(SmallFunction&& other) noexcept : vtable_{other.vtable_} {
    if (vtable_ != nullptr) {
      Move(other.storage_, storage_);
      other.Destroy();
    }
    other.vtable_ = nullptr;
  }

  SmallFunction& operator=(SmallFunction const& other) = delete;

  SmallFunction& operator=(SmallFunction&& other) noexcept {
    if (this != &other) {
      if (vtable_ != nullptr) {
        Destroy();
      }
      vtable_ = other.vtable_;
      if (vtable_ != nullptr) {
        Move(other.storage_, storage_);
        other.Destroy();
      }
      other.vtable_ = nullptr;
    }
    return *this;
  }

  /**
   * \brief Call operator
   */
  TRet operator()(TArgs... args) const {
    assert(vtable_ != nullptr && "SmallFunction is not initialized");
    return vtable_->invoke(storage_.data(), std::forward<TArgs>(args)...);
  }

  explicit operator bool() const noexcept { return vtable_ != nullptr; }

 private:
  void Destroy() noexcept {
    vtable_->manage(storage_.data(), nullptr, Operation::kDestroy);
  }

  void Move(Storage& src, Storage& dst) noexcept {
    vtable_->manage(src.data(), dst.data(), Operation::kMove);
  }

  VTable const* vtable_{};
  mutable Storage storage_{};
};

template <typename TFunction>
SmallFunction(TFunction&&)
    -> SmallFunction<typename FunctionSignature<TFunction>::Signature>;

}  // namespace ae

#endif  // AETHER_MISCPP_TYPES_SMALL_FUNCTION_H_

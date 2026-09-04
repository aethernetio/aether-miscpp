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

#ifndef AETHER_MISCPP_TYPES_METHOD_PTR_H_
#define AETHER_MISCPP_TYPES_METHOD_PTR_H_

#include <type_traits>
#include <utility>

namespace ae {
/**
 * \brief Non-owning wrapper for supported function and member-function
 * pointers.
 *
 * Member-function wrappers retain a raw instance pointer and do not manage its
 * lifetime. Only unqualified and const member functions are supported;
 * volatile-qualified and ref-qualified member functions are unsupported.
 */

template <auto M>
struct MethodPtr;

namespace method_ptr_internal {
template <typename T>
struct IsMethodPtr : std::false_type {};

template <auto Method>
struct IsMethodPtr<MethodPtr<Method>> : std::true_type {};
}  // namespace method_ptr_internal

template <typename T>
concept MethodPtrType = method_ptr_internal::IsMethodPtr<T>::value;

// Free-function pointer.
template <typename TRet, typename... TArgs, TRet (*Method)(TArgs...)>
struct MethodPtr<Method> {
  static constexpr auto kMethod = Method;
  static TRet Invoke(TArgs... args) {
    return Method(std::forward<TArgs>(args)...);
  }

  TRet operator()(TArgs... args) const {
    return Invoke(std::forward<TArgs>(args)...);
  }
};

// Unqualified class-member function pointer.
template <typename T, typename TRet, typename... TArgs,
          TRet (T::*Method)(TArgs...)>
struct MethodPtr<Method> {
  static constexpr auto kMethod = Method;
  static TRet Invoke(T* instance, TArgs... args) {
    return (instance->*Method)(std::forward<TArgs>(args)...);
  }

  TRet operator()(TArgs... args) const {
    return Invoke(instance, std::forward<TArgs>(args)...);
  }

  T* instance;
};

// Const class-member function pointer.
template <typename T, typename TRet, typename... TArgs,
          TRet (T::*Method)(TArgs...) const>
struct MethodPtr<Method> {
  static constexpr auto kMethod = Method;
  static TRet Invoke(T const* instance, TArgs... args) {
    return (instance->*Method)(std::forward<TArgs>(args)...);
  }

  TRet operator()(TArgs... args) const {
    return Invoke(instance, std::forward<TArgs>(args)...);
  }

  T const* instance;
};

}  // namespace ae

#endif  // AETHER_MISCPP_TYPES_METHOD_PTR_H_

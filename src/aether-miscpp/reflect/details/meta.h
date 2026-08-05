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

#ifndef AETHER_MISCPP_REFLECT_DETAILS_META_H_
#define AETHER_MISCPP_REFLECT_DETAILS_META_H_

#include <concepts>
#include <functional>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

#include "aether-miscpp/meta/function_signature.h"

namespace ae::reflect {
template <typename M>
concept MetaType = requires(M meta) {
  typename M::class_type;
  typename M::value_type;
  {
    meta.get(std::declval<typename M::class_type&>())
  } noexcept -> std::convertible_to<typename M::value_type const&>;
  { meta.name } -> std::convertible_to<std::string_view const&>;
};

/**
 * \brief Meta for pointer to class member
 */
template <typename C, typename T>
struct MetaMember {
  using class_type = C;
  using value_type = T;

  std::string_view name;
  T C::* member;

  template <typename U>
    requires std::is_same_v<C, std::decay_t<U>>
  decltype(auto) get(U&& value) const noexcept {
    return std::forward<U>(value).*member;
  }
};

template <typename C, typename T>
MetaMember(std::string_view, T C::* member) -> MetaMember<C, T>;

/**
 * \brief Meta for getter function of class member.
 * G getter must be invocable with C* and return value_type*
 */
template <typename C, typename G>
  requires std::is_invocable_v<G, C*>
struct MetaGetter {
  using class_type = C;
  using value_type = std::remove_pointer_t<std::invoke_result_t<G, C*>>;

  std::string_view name;
  G getter;

  template <typename U>
    requires std::is_same_v<C, std::decay_t<U>>
  decltype(auto) get(U&& value) const noexcept {
    auto* v = std::invoke(getter, const_cast<C*>(std::addressof(value)));
    if constexpr (std::is_const_v<std::remove_reference_t<U>>) {
      return std::as_const(*v);
    } else {
      return *v;
    }
  }
};

template <typename G>
MetaGetter(std::string_view, G&&)
    -> MetaGetter<std::remove_pointer_t<TypeAt_t<
                      0, typename FunctionSignature<std::decay_t<G>>::args>>,
                  std::decay_t<G>>;
}  // namespace ae::reflect

#endif  // AETHER_MISCPP_REFLECT_DETAILS_META_H_

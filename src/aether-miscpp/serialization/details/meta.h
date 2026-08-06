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

#ifndef AETHER_SERIALIZATION_DETAILS_META_H_
#define AETHER_SERIALIZATION_DETAILS_META_H_

#include <string_view>
#include <type_traits>

namespace ae::seri {
struct ForbiddenMeta;

template <typename T>
struct Meta {
  T& value;
  std::string_view name{};
};

template <typename T>
struct Meta<T const> {
  T const& value;
  std::string_view name{};
};

// forbid creation meta from rvalue
template <>
struct Meta<ForbiddenMeta>;

template <typename T>
Meta(T const&) -> Meta<std::decay_t<T> const>;
template <typename T>
Meta(T&) -> Meta<T>;
template <typename T>
Meta(T&&) -> Meta<ForbiddenMeta>;

template <typename T>
Meta(T const&, std::string_view) -> Meta<std::decay_t<T> const>;
template <typename T>
Meta(T&, std::string_view) -> Meta<T>;
template <typename T>
Meta(T&&, std::string_view) -> Meta<ForbiddenMeta>;

template <typename T>
struct IsMetaType : std::false_type {};
template <typename T>
struct IsMetaType<Meta<T>> : std::true_type {};
template <typename T>
struct IsMetaType<Meta<T> const> : std::true_type {};
template <typename T>
static inline constexpr bool IsMetaType_v = IsMetaType<T>::value;

template <typename T>
struct MetaValueType;

template <typename T>
struct MetaValueType<Meta<T>> {
  using type = T;
};
template <typename T>
struct MetaValueType<Meta<T> const> {
  using type = T;
};

template <typename T>
using MetaValueType_t = typename MetaValueType<T>::type;

}  // namespace ae::seri
#endif  // AETHER_SERIALIZATION_DETAILS_META_H_

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

#ifndef AETHER_SERIALIZATION_DETAILS_MEMBER_SERIALIZER_H_
#define AETHER_SERIALIZATION_DETAILS_MEMBER_SERIALIZER_H_

#include <concepts>

#include "aether-miscpp/serialization/details/serializer.h"

namespace ae::seri {
namespace member_serializer_internal {
template <typename Archive, typename T>
concept HasMemberSeri = requires(Archive& archive, T const& value) {
  { value.Seri(archive) } -> std::same_as<SeriResult>;
};

template <typename Archive, typename T>
concept HasMemberDeser = requires(Archive& archive, T& value) {
  { value.Deseri(archive) } -> std::same_as<SeriResult>;
};

template <typename Archive, typename T>
concept MemberSerializable =
    HasMemberSeri<Archive, T> && HasMemberDeser<Archive, T>;
}  // namespace member_serializer_internal

template <typename Archive, typename T>
  requires(member_serializer_internal::MemberSerializable<Archive, T>)
struct Serializer<Archive, T> {
  SeriResult Deseri(Archive& archive, Meta<T> val) {
    return val.value.Deseri(archive);
  }

  SeriResult Seri(Archive& archive, Meta<T const> const& val) {
    return val.value.Seri(archive);
  }
};
}  // namespace ae::seri

#endif  // AETHER_SERIALIZATION_DETAILS_MEMBER_SERIALIZER_H_

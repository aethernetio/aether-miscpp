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

#ifndef AETHER_SERIALIZATION_DETAILS_BUFFER_CONCEPT_H_
#define AETHER_SERIALIZATION_DETAILS_BUFFER_CONCEPT_H_

#include <concepts>
#include <utility>

#include "aether-miscpp/serialization/details/serialization_result.h"
#include "aether-miscpp/serialization/details/tags.h"

namespace ae::seri {

template <typename T>
concept BinaryBuffer = requires(T t) {
  {
    t.Write(std::declval<SizeWriteTag>())
  } -> std::same_as<SeriResult>;
  {
    t.Write(std::declval<DataWriteTag>())
  } -> std::same_as<SeriResult>;
  {
    t.Read(std::declval<SizeReadTag>())
  } -> std::same_as<SeriResult>;
  {
    t.Read(std::declval<DataReadTag>())
  } -> std::same_as<SeriResult>;
};

}  // namespace ae::seri

#endif  // AETHER_SERIALIZATION_DETAILS_BUFFER_CONCEPT_H_

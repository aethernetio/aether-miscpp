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
#ifndef AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_CYCLE_DETECTOR_H_
#define AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_CYCLE_DETECTOR_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <set>

#include "aether-miscpp/crc.h"
#include "aether-miscpp/meta/type_index.h"

namespace ae::domain_visitor {
inline std::size_t GetIndexFromTypeIdAndAddress(void const* obj,
                                                std::uint32_t type_index) {
  std::uint8_t buffer[sizeof(std::uintptr_t) + sizeof(type_index)]{};
  auto addr = reinterpret_cast<std::uintptr_t>(obj);
  std::memcpy(buffer, &addr, sizeof(addr));
  std::memcpy(buffer + sizeof(addr), &type_index, sizeof(type_index));
  return crc32::from_buffer(buffer, sizeof(buffer)).value;
}

template <typename T, typename _ = void>
struct ObjectIndex {
  static constexpr auto TypeIndex = GetTypeIndex<T>();

  static std::size_t GetIndex(T const* obj) {
    return GetIndexFromTypeIdAndAddress(obj, TypeIndex);
  }
};

struct CycleDetector {
  template <typename T>
  bool Add(T const* ptr) {
    return visited_objects.insert(ObjectIndex<T>::GetIndex(ptr)).second;
  }

  std::set<std::size_t> visited_objects;
};
}  // namespace ae::domain_visitor
#endif

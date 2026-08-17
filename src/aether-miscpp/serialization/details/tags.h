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

#ifndef AETHER_SERIALIZATION_DETAILS_TAGS_H_
#define AETHER_SERIALIZATION_DETAILS_TAGS_H_

#include <cstddef>

namespace ae::seri {
struct ForbiddenTag {};

struct SizeReadTag {
  std::size_t& size;
};

struct SizeWriteTag {
  std::size_t size;
};

template <typename T>
struct SizeTag : SizeWriteTag {};

template <typename T>
struct SizeTag<T&> : SizeReadTag {};

SizeTag(std::size_t&) -> SizeTag<std::size_t&>;
SizeTag(std::size_t&&) -> SizeTag<std::size_t>;
SizeTag(std::size_t const&) -> SizeTag<std::size_t>;

struct DataReadTag {
  void* data;
  std::size_t size;
};

struct DataWriteTag {
  void const* data;
  std::size_t size;
};

template <typename T>
struct DataTag : DataReadTag {
  explicit constexpr DataTag(T& d) : DataReadTag{&d, sizeof(d)} {}
};

template <typename T>
struct DataTag<T const> : DataWriteTag {
  explicit constexpr DataTag(T const& d) : DataWriteTag{&d, sizeof(d)} {}
};

template <>
struct DataTag<void> : DataReadTag {
  explicit constexpr DataTag(void* d, std::size_t s) : DataReadTag{d, s} {}
};
template <>
struct DataTag<void const> : DataWriteTag {
  explicit constexpr DataTag(void const* d, std::size_t s)
      : DataWriteTag{d, s} {}
};

template <>
struct DataTag<ForbiddenTag>;

DataTag(void*, std::size_t) -> DataTag<void>;
DataTag(void const*, std::size_t) -> DataTag<void const>;
template <typename T>
DataTag(T&) -> DataTag<T>;
template <typename T>
DataTag(T const&) -> DataTag<T const>;
template <typename T>
DataTag(T&&) -> DataTag<ForbiddenTag>;

}  // namespace ae::seri
#endif  // AETHER_SERIALIZATION_DETAILS_TAGS_H_

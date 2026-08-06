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

#ifndef AETHER_SERIALIZATION_DETAILS_SERI_BINARY_ARCHIVE_H_
#define AETHER_SERIALIZATION_DETAILS_SERI_BINARY_ARCHIVE_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "aether-miscpp/serialization/details/buffer_concept.h"
#include "aether-miscpp/serialization/details/meta.h"
#include "aether-miscpp/serialization/details/serializer.h"

namespace ae::seri {
namespace binary_archive_internal {
template <typename T, typename = void>
struct IsTrivial : std::false_type {};

template <typename T>
struct IsTrivial<
    T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
    : std::true_type {};

template <typename T>
inline constexpr bool IsTrivial_v = IsTrivial<T>::value;

template <typename Archive>
constexpr SeriResult CheckContainerLoadSize(Archive const& archive,
                                            std::size_t size) {
  if (size > archive.max_container_load_size()) {
    return Error{container_too_large};
  }
  return Ok{good};
}
}  // namespace binary_archive_internal

inline constexpr std::size_t kDefaultBinaryArchiveMaxContainerLoadSize =
    std::size_t{1024} * std::size_t{1024};

// BinaryArchive uses the platform's native in-memory representation for
// trivially copyable values. It is not a portable wire format; callers are
// responsible for choosing fixed-width types when portability matters.
// bool values are encoded as 0x00/0x01. max_container_load_size is enforced
// after the size prefix has been read. Generic std serializers treat it as an
// element-count cap, while optimized vector/string binary loads also reject
// payload byte counts that exceed the cap before allocation.

template <typename Buffer>
  requires(BinaryBuffer<Buffer>)
class BinaryArchive {
 public:
  explicit BinaryArchive(Buffer const& buffer,
                         std::size_t max_container_load_size =
                             kDefaultBinaryArchiveMaxContainerLoadSize)
      : buffer_(buffer), max_container_load_size_(max_container_load_size) {}

  explicit BinaryArchive(Buffer&& buffer) noexcept(
      std::is_nothrow_move_constructible_v<Buffer>)
      : buffer_(std::move(buffer)),
        max_container_load_size_(kDefaultBinaryArchiveMaxContainerLoadSize) {}

  BinaryArchive(Buffer&& buffer, std::size_t max_container_load_size) noexcept(
      std::is_nothrow_move_constructible_v<Buffer>)
      : buffer_(std::move(buffer)),
        max_container_load_size_(max_container_load_size) {}

  Buffer& buffer() & noexcept { return buffer_; }

  Buffer const& buffer() const& noexcept { return buffer_; }

  Buffer&& buffer() && noexcept { return std::move(buffer_); }

  std::size_t max_container_load_size() const noexcept {
    return max_container_load_size_;
  }

  template <typename T>
  SeriResult Save(Meta<T const> const& meta) {
    using value_type = std::remove_const_t<MetaValueType_t<Meta<T const>>>;
    return Serializer<BinaryArchive<Buffer>, value_type>{}.Seri(*this, meta);
  }

  template <typename T>
    requires(!IsMetaType_v<T>)
  SeriResult Save(T const& value) {
    return Save(Meta{value, ""});
  }

  template <typename T>
  SeriResult Load(Meta<T> meta) {
    using value_type = MetaValueType_t<Meta<T>>;
    return Serializer<BinaryArchive<Buffer>, value_type>{}.Deseri(*this, meta);
  }

  template <typename T>
    requires(!IsMetaType_v<T>)
  SeriResult Load(T& value) {
    return Load(Meta{value, ""});
  }

 private:
  Buffer buffer_;
  std::size_t max_container_load_size_{};
};
template <typename B>
BinaryArchive(B&&) -> BinaryArchive<std::decay_t<B>>;
template <typename B>
BinaryArchive(B&&, std::size_t) -> BinaryArchive<std::decay_t<B>>;

template <typename B>
struct Serializer<BinaryArchive<B>, bool> {
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<bool> val) {
    std::uint8_t raw{};
    TRY_RESULT(archive.buffer().Read(DataTag{raw}));
    if (raw == std::uint8_t{0x00}) {
      val.value = false;
    } else if (raw == std::uint8_t{0x01}) {
      val.value = true;
    } else {
      return Error{invalid_bool};
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive, Meta<bool const> const& val) {
    auto const raw = val.value ? std::uint8_t{0x01} : std::uint8_t{0x00};
    return archive.buffer().Write(DataTag{raw});
  }
};

template <typename B, typename T>
  requires(binary_archive_internal::IsTrivial_v<T> && !std::is_same_v<T, bool>)
struct Serializer<BinaryArchive<B>, T> {
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<T> val) {
    return archive.buffer().Read(DataTag{val.value});
  }
  SeriResult Seri(BinaryArchive<B>& archive, Meta<T const> const& val) {
    return archive.buffer().Write(DataTag{val.value});
  }
};

template <typename B, typename T>
  requires(std::is_enum_v<T>)
struct Serializer<BinaryArchive<B>, T> {
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<T> val) {
    auto underlying = std::underlying_type_t<T>{};
    TRY_RESULT(archive.buffer().Read(DataTag{underlying}));
    val.value = static_cast<T>(underlying);
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive, Meta<T const> const& val) {
    auto const underlying = static_cast<std::underlying_type_t<T>>(val.value);
    return archive.buffer().Write(DataTag{underlying});
  }
};

template <typename B, typename T, std::size_t S>
  requires(binary_archive_internal::IsTrivial_v<T>)
struct Serializer<BinaryArchive<B>, T[S]> {
  using Arr = T[S];
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<Arr> val) {
    return archive.buffer().Read(DataReadTag{
        reinterpret_cast<std::uint8_t*>(&val.value[0]), S * sizeof(T)});
  }

  SeriResult Seri(BinaryArchive<B>& archive, Meta<Arr const> const& val) {
    return archive.buffer().Write(DataWriteTag{
        reinterpret_cast<std::uint8_t const*>(&val.value[0]), S * sizeof(T)});
  }
};

}  // namespace ae::seri

#endif

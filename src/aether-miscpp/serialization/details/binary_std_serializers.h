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

#ifndef AETHER_SERIALIZATION_DETAILS_BINARY_STD_SERIALIZERS_H_
#define AETHER_SERIALIZATION_DETAILS_BINARY_STD_SERIALIZERS_H_

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "aether-miscpp/serialization/details/binary_archive.h"

namespace ae::seri {

namespace binary_std_serializers_internal {

using binary_archive_internal::CheckContainerLoadSize;

template <typename Archive>
SeriResult CheckContainerLoadBytes(Archive const& archive, std::size_t size,
                                   std::size_t element_size) noexcept {
  assert(element_size != 0);
  if (size > (archive.max_container_load_size() / element_size)) {
    return Error{container_too_large};
  }
  return Ok{good};
}

template <typename T>
inline constexpr bool IsBinaryRawVectorElement_v =
    binary_archive_internal::IsTrivial_v<T> || std::is_enum_v<T>;

}  // namespace binary_std_serializers_internal

template <typename B, typename T, typename Allocator>
struct Serializer<BinaryArchive<B>, std::vector<T, Allocator>> {
  SeriResult Deseri(BinaryArchive<B>& archive,
                    Meta<std::vector<T, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    if constexpr (binary_std_serializers_internal::IsBinaryRawVectorElement_v<
                      T>) {
      TRY_RESULT(binary_std_serializers_internal::CheckContainerLoadBytes(
          archive, size, sizeof(T)));
      val.value.resize(size);
      if (size > 0) {
        auto* data = reinterpret_cast<std::uint8_t*>(val.value.data());
        TRY_RESULT(archive.buffer().Read(DataReadTag{data, size * sizeof(T)}));
      }
    } else {
      TRY_RESULT(binary_std_serializers_internal::CheckContainerLoadSize(
          archive, size));
      val.value.resize(size);
      for (auto& elem : val.value) {
        TRY_RESULT(archive.Load(elem));
      }
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::vector<T, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    if constexpr (binary_std_serializers_internal::IsBinaryRawVectorElement_v<
                      T>) {
      if (!val.value.empty()) {
        auto const* data =
            reinterpret_cast<std::uint8_t const*>(val.value.data());
        TRY_RESULT(archive.buffer().Write(
            DataWriteTag{data, val.value.size() * sizeof(T)}));
      }
    } else {
      for (auto const& elem : val.value) {
        TRY_RESULT(archive.Save(elem));
      }
    }
    return Ok{good};
  }
};

template <typename B, typename Allocator>
struct Serializer<BinaryArchive<B>, std::vector<bool, Allocator>> {
  SeriResult Deseri(BinaryArchive<B>& archive,
                    Meta<std::vector<bool, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.resize(size);
    for (std::size_t i = 0; i < val.value.size(); ++i) {
      auto bit = false;
      TRY_RESULT(archive.Load(bit));
      val.value[i] = bit;
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::vector<bool, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& elem : val.value) {
      auto bit = static_cast<bool>(elem);
      TRY_RESULT(archive.Save(bit));
    }
    return Ok{good};
  }
};

template <typename B, typename T, typename Allocator>
struct Serializer<BinaryArchive<B>, std::list<T, Allocator>> {
  SeriResult Deseri(BinaryArchive<B>& archive,
                    Meta<std::list<T, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      T elem{};
      TRY_RESULT(archive.Load(elem));
      val.value.push_back(std::move(elem));
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::list<T, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& elem : val.value) {
      TRY_RESULT(archive.Save(elem));
    }
    return Ok{good};
  }
};

template <typename B, typename T, typename Allocator>
struct Serializer<BinaryArchive<B>, std::deque<T, Allocator>> {
  SeriResult Deseri(BinaryArchive<B>& archive,
                    Meta<std::deque<T, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      T elem{};
      TRY_RESULT(archive.Load(elem));
      val.value.push_back(std::move(elem));
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::deque<T, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& elem : val.value) {
      TRY_RESULT(archive.Save(elem));
    }
    return Ok{good};
  }
};

template <typename B, typename T, typename Compare, typename Allocator>
struct Serializer<BinaryArchive<B>, std::set<T, Compare, Allocator>> {
  SeriResult Deseri(BinaryArchive<B>& archive,
                    Meta<std::set<T, Compare, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      T elem{};
      TRY_RESULT(archive.Load(elem));
      val.value.insert(std::move(elem));
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::set<T, Compare, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& elem : val.value) {
      TRY_RESULT(archive.Save(elem));
    }
    return Ok{good};
  }
};

template <typename B, typename T, typename Compare, typename Allocator>
struct Serializer<BinaryArchive<B>, std::multiset<T, Compare, Allocator>> {
  SeriResult Deseri(BinaryArchive<B>& archive,
                    Meta<std::multiset<T, Compare, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      T elem{};
      TRY_RESULT(archive.Load(elem));
      val.value.insert(std::move(elem));
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::multiset<T, Compare, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& elem : val.value) {
      TRY_RESULT(archive.Save(elem));
    }
    return Ok{good};
  }
};

template <typename B, typename T, typename Hash, typename KeyEqual,
          typename Allocator>
struct Serializer<BinaryArchive<B>,
                  std::unordered_set<T, Hash, KeyEqual, Allocator>> {
  SeriResult Deseri(
      BinaryArchive<B>& archive,
      Meta<std::unordered_set<T, Hash, KeyEqual, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      T elem{};
      TRY_RESULT(archive.Load(elem));
      val.value.insert(std::move(elem));
    }
    return Ok{good};
  }
  SeriResult Seri(
      BinaryArchive<B>& archive,
      Meta<std::unordered_set<T, Hash, KeyEqual, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& elem : val.value) {
      TRY_RESULT(archive.Save(elem));
    }
    return Ok{good};
  }
};

template <typename B, typename T, typename Hash, typename KeyEqual,
          typename Allocator>
struct Serializer<BinaryArchive<B>,
                  std::unordered_multiset<T, Hash, KeyEqual, Allocator>> {
  SeriResult Deseri(
      BinaryArchive<B>& archive,
      Meta<std::unordered_multiset<T, Hash, KeyEqual, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      T elem{};
      TRY_RESULT(archive.Load(elem));
      val.value.insert(std::move(elem));
    }
    return Ok{good};
  }
  SeriResult Seri(
      BinaryArchive<B>& archive,
      Meta<std::unordered_multiset<T, Hash, KeyEqual, Allocator> const> const&
          val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& elem : val.value) {
      TRY_RESULT(archive.Save(elem));
    }
    return Ok{good};
  }
};

template <typename B, typename Key, typename T, typename Compare,
          typename Allocator>
struct Serializer<BinaryArchive<B>, std::map<Key, T, Compare, Allocator>> {
  SeriResult Deseri(BinaryArchive<B>& archive,
                    Meta<std::map<Key, T, Compare, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      std::pair<Key, T> entry{};
      TRY_RESULT(archive.Load(entry));
      val.value.emplace(std::move(entry));
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::map<Key, T, Compare, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& entry : val.value) {
      TRY_RESULT(archive.Save(entry));
    }
    return Ok{good};
  }
};

template <typename B, typename Key, typename T, typename Compare,
          typename Allocator>
struct Serializer<BinaryArchive<B>, std::multimap<Key, T, Compare, Allocator>> {
  SeriResult Deseri(BinaryArchive<B>& archive,
                    Meta<std::multimap<Key, T, Compare, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      std::pair<Key, T> entry{};
      TRY_RESULT(archive.Load(entry));
      val.value.emplace(std::move(entry));
    }
    return Ok{good};
  }
  SeriResult Seri(
      BinaryArchive<B>& archive,
      Meta<std::multimap<Key, T, Compare, Allocator> const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& entry : val.value) {
      TRY_RESULT(archive.Save(entry));
    }
    return Ok{good};
  }
};

template <typename B, typename Key, typename T, typename Hash,
          typename KeyEqual, typename Allocator>
struct Serializer<BinaryArchive<B>,
                  std::unordered_map<Key, T, Hash, KeyEqual, Allocator>> {
  SeriResult Deseri(
      BinaryArchive<B>& archive,
      Meta<std::unordered_map<Key, T, Hash, KeyEqual, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      std::pair<Key, T> entry{};
      TRY_RESULT(archive.Load(entry));
      val.value.emplace(std::move(entry));
    }
    return Ok{good};
  }
  SeriResult Seri(
      BinaryArchive<B>& archive,
      Meta<std::unordered_map<Key, T, Hash, KeyEqual, Allocator> const> const&
          val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& entry : val.value) {
      TRY_RESULT(archive.Save(entry));
    }
    return Ok{good};
  }
};

template <typename B, typename Key, typename T, typename Hash,
          typename KeyEqual, typename Allocator>
struct Serializer<BinaryArchive<B>,
                  std::unordered_multimap<Key, T, Hash, KeyEqual, Allocator>> {
  SeriResult Deseri(
      BinaryArchive<B>& archive,
      Meta<std::unordered_multimap<Key, T, Hash, KeyEqual, Allocator>> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(
        binary_std_serializers_internal::CheckContainerLoadSize(archive, size));
    val.value.clear();
    for (std::size_t i = 0; i < size; ++i) {
      std::pair<Key, T> entry{};
      TRY_RESULT(archive.Load(entry));
      val.value.emplace(std::move(entry));
    }
    return Ok{good};
  }
  SeriResult Seri(
      BinaryArchive<B>& archive,
      Meta<std::unordered_multimap<Key, T, Hash, KeyEqual, Allocator> const>
          val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    for (auto const& entry : val.value) {
      TRY_RESULT(archive.Save(entry));
    }
    return Ok{good};
  }
};

template <typename B, typename CharT, typename Traits, typename Allocator>
struct Serializer<BinaryArchive<B>,
                  std::basic_string<CharT, Traits, Allocator>> {
  using String = std::basic_string<CharT, Traits, Allocator>;
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<String> val) {
    std::size_t size{};
    TRY_RESULT(archive.buffer().Read(SizeTag{size}));
    TRY_RESULT(binary_std_serializers_internal::CheckContainerLoadBytes(
        archive, size, sizeof(CharT)));
    val.value.resize(size);
    auto* data = reinterpret_cast<std::uint8_t*>(val.value.data());
    TRY_RESULT(archive.buffer().Read(DataReadTag{data, size * sizeof(CharT)}));
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive, Meta<String const> const& val) {
    auto const size = val.value.size();
    TRY_RESULT(archive.buffer().Write(SizeTag{size}));
    auto const* data = reinterpret_cast<std::uint8_t const*>(val.value.data());
    TRY_RESULT(
        archive.buffer().Write(DataWriteTag{data, size * sizeof(CharT)}));
    return Ok{good};
  }
};

template <typename B, typename T>
struct Serializer<BinaryArchive<B>, std::optional<T>> {
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<std::optional<T>> val) {
    bool engaged{};
    TRY_RESULT(archive.Load(engaged));
    if (!engaged) {
      val.value.reset();
      return Ok{good};
    }
    TRY_RESULT(archive.Load(val.value.emplace()));
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::optional<T> const> const& val) {
    TRY_RESULT(archive.Save(val.value.has_value()));
    if (val.value.has_value()) {
      TRY_RESULT(archive.Save(*val.value));
    }
    return Ok{good};
  }
};

template <typename B, typename T, typename U>
struct Serializer<BinaryArchive<B>, std::pair<T, U>> {
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<std::pair<T, U>> val) {
    TRY_RESULT(archive.Load(val.value.first));
    TRY_RESULT(archive.Load(val.value.second));
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::pair<T, U> const> const& val) {
    TRY_RESULT(archive.Save(val.value.first));
    TRY_RESULT(archive.Save(val.value.second));
    return Ok{good};
  }
};

template <typename B, typename T, std::size_t S>
struct Serializer<BinaryArchive<B>, std::array<T, S>> {
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<std::array<T, S>> val) {
    if constexpr (binary_archive_internal::IsTrivial_v<T> ||
                  std::is_enum_v<T>) {
      if constexpr (S > 0) {
        TRY_RESULT(archive.buffer().Read(DataReadTag{
            reinterpret_cast<std::uint8_t*>(val.value.data()), S * sizeof(T)}));
      }
    } else {
      for (auto& elem : val.value) {
        TRY_RESULT(archive.Load(elem));
      }
    }
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::array<T, S> const> const& val) {
    if constexpr (binary_archive_internal::IsTrivial_v<T> ||
                  std::is_enum_v<T>) {
      if constexpr (S > 0) {
        TRY_RESULT(archive.buffer().Write(DataWriteTag{
            reinterpret_cast<std::uint8_t const*>(val.value.data()),
            S * sizeof(T)}));
      }
    } else {
      for (auto const& elem : val.value) {
        TRY_RESULT(archive.Save(elem));
      }
    }
    return Ok{good};
  }
};

template <typename B, typename... Ts>
struct Serializer<BinaryArchive<B>, std::tuple<Ts...>> {
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<std::tuple<Ts...>> val) {
    if constexpr (sizeof...(Ts) == 0) {
      return Ok{good};
    } else {
      return std::apply(
          [&](auto&... elems) {
            SeriResult result = Ok{good};
            auto _ = ((result = archive.Load(elems), !!result) && ...);
            (void)_;
            return result;
          },
          val.value);
    }
  }
  SeriResult Seri(BinaryArchive<B>& archive,
                  Meta<std::tuple<Ts...> const> const& val) {
    if constexpr (sizeof...(Ts) == 0) {
      return Ok{good};
    } else {
      return std::apply(
          [&](auto const&... elems) {
            SeriResult result = Ok{good};
            auto _ = ((result = archive.Save(elems), !!result) && ...);
            (void)_;
            return result;
          },
          val.value);
    }
  }
};

template <typename B, typename... Ts>
struct Serializer<BinaryArchive<B>, std::variant<Ts...>> {
  using Variant = std::variant<Ts...>;
  static_assert(std::variant_size_v<Variant> <= 256);

  SeriResult Deseri(BinaryArchive<B>& archive, Meta<Variant> val) {
    std::uint8_t index{};
    TRY_RESULT(archive.Load(index));
    if (index >= std::variant_size_v<Variant>) {
      return Error{invalid_variant_index};
    }
    return LoadIndex<0>(archive, val.value, index);
  }
  SeriResult Seri(BinaryArchive<B>& archive, Meta<Variant const> const& val) {
    if (val.value.valueless_by_exception()) {
      return Error{invalid_variant_index};
    }
    TRY_RESULT(archive.Save(static_cast<std::uint8_t>(val.value.index())));
    return SaveIndex<0>(archive, val.value);
  }

 private:
  template <std::size_t I>
  SeriResult LoadIndex(BinaryArchive<B>& archive, Variant& value,
                       std::size_t index) {
    if constexpr (I >= std::variant_size_v<Variant>) {
      return Error{invalid_variant_index};
    } else {
      if (index == I) {
        value.template emplace<I>();
        TRY_RESULT(archive.Load(std::get<I>(value)));
        return Ok{good};
      }
      return LoadIndex<I + 1>(archive, value, index);
    }
  }
  template <std::size_t I>
  SeriResult SaveIndex(BinaryArchive<B>& archive, Variant const& value) {
    if constexpr (I >= std::variant_size_v<Variant>) {
      return Error{invalid_variant_index};
    } else {
      if (value.index() == I) {
        return archive.Save(std::get<I>(value));
      }
      return SaveIndex<I + 1>(archive, value);
    }
  }
};

template <typename B, typename Rep, typename Period>
struct Serializer<BinaryArchive<B>, std::chrono::duration<Rep, Period>> {
  using Duration = std::chrono::duration<Rep, Period>;
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<Duration> val) {
    std::uint64_t count{};
    TRY_RESULT(archive.Load(count));
    auto const ms = std::chrono::milliseconds{count};
    val.value = std::chrono::duration_cast<Duration>(ms);
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive, Meta<Duration const> const& val) {
    auto const ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(val.value);
    auto const ms_count = static_cast<std::uint64_t>(ms.count());
    return archive.Save(ms_count);
  }
};

template <typename B, typename Clock, typename Duration>
struct Serializer<BinaryArchive<B>, std::chrono::time_point<Clock, Duration>> {
  using TimePoint = std::chrono::time_point<Clock, Duration>;
  SeriResult Deseri(BinaryArchive<B>& archive, Meta<TimePoint> val) {
    std::uint64_t since_epoch{};
    TRY_RESULT(archive.Load(since_epoch));
    auto const ms = std::chrono::milliseconds{since_epoch};
    val.value = TimePoint{std::chrono::duration_cast<Duration>(ms)};
    return Ok{good};
  }
  SeriResult Seri(BinaryArchive<B>& archive, Meta<TimePoint const> const& val) {
    auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        val.value.time_since_epoch());
    auto const ms_count = static_cast<std::uint64_t>(ms.count());
    return archive.Save(ms_count);
  }
};

}  // namespace ae::seri

#endif  // AETHER_SERIALIZATION_DETAILS_BINARY_STD_SERIALIZERS_H_

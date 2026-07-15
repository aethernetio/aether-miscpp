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

#ifndef AETHER_MISCPP_FORMAT_DEFAULT_FORMATTERS_H_
#define AETHER_MISCPP_FORMAT_DEFAULT_FORMATTERS_H_

#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "aether-miscpp/format/format_impl.h"
#include "aether-miscpp/format/formatter.h"
#include "aether-miscpp/format/numeric_helpers.h"

namespace ae {
namespace format_internal {

template <typename T>
using BareT = std::remove_cvref_t<T>;

template <typename T>
concept StringLike = std::same_as<BareT<T>, std::string> ||
                     std::same_as<BareT<T>, std::string_view>;

template <typename T>
concept HasValueType = requires { typename BareT<T>::value_type; };

template <typename T>
concept ContainerLike = !StringLike<T> && HasValueType<T> &&
                        std::ranges::input_range<BareT<T> const&>;

template <typename T>
concept ByteBufferContainer =
    ContainerLike<T> &&
    std::same_as<typename BareT<T>::value_type, std::uint8_t>;

template <typename Writer>
void WriteUnsignedIntegral(Writer& writer, std::uint64_t value) {
  WriteUnsigned(writer, value);
}

template <typename Writer>
void WriteSignedIntegral(Writer& writer, std::int64_t value) {
  if (value < 0) {
    writer.write('-');
    auto magnitude = std::uint64_t{0} - static_cast<std::uint64_t>(value);
    WriteUnsignedIntegral(writer, magnitude);
  } else {
    WriteUnsignedIntegral(writer, static_cast<std::uint64_t>(value));
  }
}

template <typename Writer, typename T>
void WriteIntegral(Writer& writer, T value) {
  if constexpr (std::is_signed_v<T>) {
    WriteSignedIntegral(writer, static_cast<std::int64_t>(value));
  } else {
    WriteUnsignedIntegral(writer, static_cast<std::uint64_t>(value));
  }
}

template <typename Writer, typename T>
void WriteFloat(Writer& writer, T value) {
  constexpr auto kFloatBufferSize = std::size_t{64};
  auto buff = std::array<char, kFloatBufferSize>{};
  auto [ptr, ec] = std::to_chars(buff.data(), buff.data() + buff.size(), value,
                                 std::chars_format::general);
  if (ec == std::errc{}) {
    writer.write(std::string_view{buff.data(),
                                  static_cast<std::size_t>(ptr - buff.data())});
    return;
  }
  writer.write("{float error ");
  WriteSignedIntegral(writer, static_cast<std::int64_t>(static_cast<int>(ec)));
  writer.write('}');
}

}  // namespace format_internal

template <>
struct Formatter<std::nullptr_t> {
  template <typename TStream>
  void Format(std::nullptr_t, FormatContext<TStream>& ctx) const {
    ctx.out().write("(null)");
  }
};

template <>
struct Formatter<std::string> {
  template <typename TStream>
  void Format(std::string const& value, FormatContext<TStream>& ctx) const {
    ctx.out().write(value);
  }
};

template <>
struct Formatter<std::string_view> {
  template <typename TStream>
  void Format(std::string_view value, FormatContext<TStream>& ctx) const {
    ctx.out().write(value);
  }
};

template <>
struct Formatter<char const*> {
  template <typename TStream>
  void Format(char const* value, FormatContext<TStream>& ctx) const {
    ctx.out().write(value != nullptr ? std::string_view{value}
                                     : std::string_view{"(null)"});
  }
};

template <>
struct Formatter<char*> : Formatter<char const*> {};

template <>
struct Formatter<char> {
  template <typename TStream>
  void Format(char value, FormatContext<TStream>& ctx) const {
    ctx.out().write(value);
  }
};

template <>
struct Formatter<bool> {
  template <typename TStream>
  void Format(bool value, FormatContext<TStream>& ctx) const {
    ctx.out().write(value ? "true" : "false");
  }
};

template <typename T>
struct Formatter<T, std::enable_if_t<std::is_enum_v<T>>> {
  template <typename TStream>
  void Format(T value, FormatContext<TStream>& ctx) const {
    format_internal::WriteIntegral(
        ctx.out(), static_cast<std::underlying_type_t<T>>(value));
  }
};

template <typename T>
struct Formatter<
    T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> &&
                        !std::is_same_v<T, char>>> {
  template <typename TStream>
  void Format(T value, FormatContext<TStream>& ctx) const {
    format_internal::WriteIntegral(ctx.out(), value);
  }
};

template <typename T>
struct Formatter<T, std::enable_if_t<std::is_floating_point_v<T>>> {
  template <typename TStream>
  void Format(T value, FormatContext<TStream>& ctx) const {
    format_internal::WriteFloat(ctx.out(), value);
  }
};

template <typename T>
struct Formatter<std::optional<T>> {
  template <typename TStream>
    requires format_internal::HasFormatterFor<T, TStream>
  void Format(std::optional<T> const& value,
              FormatContext<TStream>& ctx) const {
    if (!value) {
      ctx.out().write("nullopt");
    } else {
      format_internal::FormatValue(ctx.out(), *value, {});
    }
  }
};

template <typename T>
struct Formatter<T, std::enable_if_t<format_internal::ByteBufferContainer<T>>> {
  template <typename TStream>
  void Format(T const& value, FormatContext<TStream>& ctx) const {
    static constexpr auto kHex = std::string_view{"0123456789abcdef"};
    static constexpr auto kHexNibbleMask = std::uint8_t{0x0f};
    ctx.out().write("0x");
    for (auto byte : value) {
      auto const byte_value = static_cast<std::uint8_t>(byte);
      auto buffer = std::array<char, 2>{
          kHex[(byte_value >> 4) & kHexNibbleMask],  // NOLINT(*bounds*)
          kHex[byte_value & kHexNibbleMask]};        // NOLINT(*bounds*)
      ctx.out().write(std::string_view{buffer.data(), buffer.size()});
    }
  }
};

template <typename T>
struct Formatter<T,
                 std::enable_if_t<format_internal::ContainerLike<T> &&
                                   !format_internal::ByteBufferContainer<T>>> {
  template <typename TStream>
    requires format_internal::HasFormatterFor<
        std::ranges::range_reference_t<format_internal::BareT<T> const&>,
        TStream>
  void Format(T const& value, FormatContext<TStream>& ctx) const {
    bool first = true;
    for (auto const& item : value) {
      if (!first) {
        ctx.out().write(", ");
      }
      first = false;
      if constexpr (format_internal::ContainerLike<decltype(item)> &&
                    !format_internal::ByteBufferContainer<decltype(item)>) {
        ctx.out().write('[');
        format_internal::FormatValue(ctx.out(), item, {});
        ctx.out().write(']');
      } else {
        format_internal::FormatValue(ctx.out(), item, {});
      }
    }
  }
};

}  // namespace ae

#endif  // AETHER_MISCPP_FORMAT_DEFAULT_FORMATTERS_H_

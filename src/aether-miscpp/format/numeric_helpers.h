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

#ifndef AETHER_MISCPP_FORMAT_NUMERIC_HELPERS_H_
#define AETHER_MISCPP_FORMAT_NUMERIC_HELPERS_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>

namespace ae::format_internal {

inline constexpr auto kDecimalBase = 10;

template <int Base, typename Magnitude>
Magnitude TakeRemainder(Magnitude& value) noexcept {
  static_assert(std::is_unsigned_v<Magnitude>);
  if constexpr (std::cmp_less(std::numeric_limits<Magnitude>::max(), Base)) {
    auto const rem = value;
    value = 0;
    return rem;
  } else {
    auto const divisor = static_cast<Magnitude>(Base);
    auto const rem = static_cast<Magnitude>(value % divisor);
    value = static_cast<Magnitude>(value / divisor);
    return rem;
  }
}

template <typename Writer, typename Magnitude>
void WriteUnsigned(Writer& out, Magnitude value) {
  static_assert(std::is_unsigned_v<Magnitude>);
  if (value == 0) {
    out.write(std::string_view{"0"});
    return;
  }
  auto buff = std::array<char, std::numeric_limits<Magnitude>::digits10 + 2>{};
  auto wp = std::size_t{};
  while (value != 0) {
    auto const digit = TakeRemainder<kDecimalBase>(value);
    buff[wp++] = static_cast<char>('0' + digit);  // NOLINT(*bounds*)
  }
  std::reverse(buff.begin(), buff.begin() + static_cast<std::ptrdiff_t>(wp));
  out.write(std::string_view{buff.data(), wp});
}

// Writes an unsigned magnitude with zero padding. width is a minimum, not a
// maximum; values longer than width are not truncated. Callers must pass an
// unsigned Magnitude and keep width <= numeric_limits<Magnitude>::digits10 + 2
// because this function uses a fixed local buffer. No runtime width check is
// performed.
template <typename Writer, typename Magnitude>
void WritePaddedUnsigned(Writer& out, Magnitude value, std::size_t width) {
  static_assert(std::is_unsigned_v<Magnitude>);
  auto buff = std::array<char, std::numeric_limits<Magnitude>::digits10 + 2>{};
  auto wp = std::size_t{};
  if (value == 0) {
    buff[wp++] = '0';  // NOLINT(*bounds*)
  }
  while (value != 0) {
    auto const digit = TakeRemainder<kDecimalBase>(value);
    buff[wp++] = static_cast<char>('0' + digit);  // NOLINT(*bounds*)
  }
  while (wp < width) {
    buff[wp++] = '0';  // NOLINT(*bounds*)
  }
  std::reverse(buff.begin(), buff.begin() + static_cast<std::ptrdiff_t>(wp));
  out.write(std::string_view{buff.data(), wp});
}

}  // namespace ae::format_internal

#endif  // AETHER_MISCPP_FORMAT_NUMERIC_HELPERS_H_

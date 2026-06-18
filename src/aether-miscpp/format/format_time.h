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
#ifndef AETHER_MISCPP_FORMAT_FORMAT_TIME_H_
#define AETHER_MISCPP_FORMAT_FORMAT_TIME_H_

#include <chrono>
#include <string>
#include <format>

#include "aether-miscpp/format/formatter.h"

namespace ae {
namespace format_internal {

inline std::string FormatTimeWithOptions(std::string_view options,
                                         auto duration_us) {
  // Convert to sys_days for calendar decomposition
  auto tp_us = std::chrono::system_clock::time_point{duration_us};
  auto sd = std::chrono::floor<std::chrono::days>(tp_us);
  auto ymd = std::chrono::year_month_day{sd};
  std::chrono::hh_mm_ss hms{tp_us - sd};

  auto h = hms.hours().count();
  auto m = hms.minutes().count();
  auto s = hms.seconds().count();
  auto sub =
      std::chrono::duration_cast<std::chrono::microseconds>(hms.subseconds())
          .count();

  auto yr = static_cast<int>(ymd.year());
  auto mo = static_cast<unsigned>(ymd.month());
  auto dy = static_cast<unsigned>(ymd.day());

  std::string result;
  auto inserter = std::back_inserter(result);

  for (std::size_t i = 0; i < options.size(); ++i) {
    if (options[i] == '%' && i + 1 < options.size()) {
      ++i;
      switch (options[i]) {
        case 'H':
          std::format_to(inserter, "{:02}", h);
          break;
        case 'M':
          std::format_to(inserter, "{:02}", m);
          break;
        case 'S':
          std::format_to(inserter, "{:02}.{:06}", s, sub);
          break;
        case 'Y':
          std::format_to(inserter, "{:04}", yr);
          break;
        case 'm':
          std::format_to(inserter, "{:02}", mo);
          break;
        case 'd':
          std::format_to(inserter, "{:02}", dy);
          break;
        case 'f':
          std::format_to(inserter, "{:06}", sub);
          break;
        case 'F':
          std::format_to(inserter, "{:04}-{:02}-{:02}", yr, mo, dy);
          break;
        case 'T':
          std::format_to(inserter, "{:02}:{:02}:{:02}.{:06}", h, m, s, sub);
          break;
        case '%':
          result += '%';
          break;
        default:
          result += '%';
          result += options[i];
          break;
      }
    } else {
      result += options[i];
    }
  }
  return result;
}

}  // namespace format_internal

/**
 * \brief Format TimePoint to string.
 * Uses C++20 std::chrono facilities (hh_mm_ss) for decomposition
 * and std::format_to for formatting individual components.
 */
template <typename C, typename D>
struct Formatter<std::chrono::time_point<C, D>> {
  template <typename TStream>
  void Format(std::chrono::time_point<C, D> const& value,
              FormatContext<TStream>& ctx) const {
    auto tp_us = std::chrono::time_point_cast<std::chrono::microseconds>(value);
    auto result = format_internal::FormatTimeWithOptions(
        ctx.options, tp_us.time_since_epoch());
    ctx.out().write(result);
  }
};

template <typename Rep, typename Per>
struct Formatter<std::chrono::duration<Rep, Per>> {
  template <typename TStream>
  void Format(std::chrono::duration<Rep, Per> const& value,
              FormatContext<TStream>& ctx) const {
    bool has_spec = !ctx.options.empty();
    if (!has_spec) {
      auto count =
          std::chrono::duration_cast<std::chrono::duration<Rep, Per>>(value)
              .count();
      ctx.out().write(std::to_string(count));
    } else {
      auto us = std::chrono::duration_cast<std::chrono::microseconds>(value);
      auto tp = std::chrono::system_clock::time_point{
          std::chrono::duration_cast<std::chrono::system_clock::duration>(us)};
      auto result = format_internal::FormatTimeWithOptions(ctx.options, tp);
      ctx.out().write(result);
    }
  }
};

}  // namespace ae
#endif  // AETHER_MISCPP_FORMAT_FORMAT_TIME_H_

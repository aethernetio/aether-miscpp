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
#include <cstddef>
#include <ratio>
#include <type_traits>

#include "aether-miscpp/format/formatter.h"
#include "aether-miscpp/format/numeric_helpers.h"

namespace ae {
namespace format_internal {

inline constexpr auto kSubsecondBase = 1000;
inline constexpr auto kSecondsPerMinute = 60;
inline constexpr auto kMinutesPerHour = 60;
inline constexpr auto kSecondsPerHour = 3600;
inline constexpr auto kTwoDigitWidth = std::size_t{2};
inline constexpr auto kSubsecondChunkWidth = std::size_t{3};
using HoursPeriod = std::ratio<kSecondsPerHour>;
using MinutesPeriod = std::ratio<kSecondsPerMinute>;
using SecondsPeriod = std::ratio<1>;

template <typename Rep, typename Period>
// Supported builtin duration periods:
//   ratio<3600>  - hours
//   ratio<60>    - minutes
//   ratio<1>     - seconds
//   std::milli   - milliseconds
//   std::micro   - microseconds
//   std::nano    - nanoseconds
concept SupportedDuration =
    std::is_integral_v<Rep> && !std::is_same_v<Rep, bool> &&
    (std::ratio_equal_v<Period, HoursPeriod> ||
     std::ratio_equal_v<Period, MinutesPeriod> ||
     std::ratio_equal_v<Period, SecondsPeriod> ||
     std::ratio_equal_v<Period, std::milli> ||
     std::ratio_equal_v<Period, std::micro> ||
     std::ratio_equal_v<Period, std::nano>);

template <typename Writer, typename Magnitude>
void WriteHourMinuteSecond(Writer& writer, Magnitude hours, Magnitude minutes,
                           Magnitude seconds, bool pad_hours) {
  if (pad_hours) {
    WritePaddedUnsigned(writer, hours, kTwoDigitWidth);
  } else {
    WriteUnsigned(writer, hours);
  }
  writer.write(':');
  WritePaddedUnsigned(writer, minutes, kTwoDigitWidth);
  writer.write(':');
  WritePaddedUnsigned(writer, seconds, kTwoDigitWidth);
}

template <typename Writer>
void WriteTimeOfDayMicros(Writer& writer,
                          std::chrono::hh_mm_ss<std::chrono::microseconds> const& tod) {
  WriteHourMinuteSecond(writer, static_cast<unsigned>(tod.hours().count()),
                        static_cast<unsigned>(tod.minutes().count()),
                        static_cast<unsigned>(tod.seconds().count()), true);
  writer.write('.');
  WritePaddedUnsigned(
      writer,
      static_cast<unsigned>(
          std::chrono::duration_cast<std::chrono::microseconds>(tod.subseconds())
              .count()),
      2 * kSubsecondChunkWidth);
}

template <typename Writer>
void WriteChronoDateTime(Writer& writer,
                         std::chrono::system_clock::time_point tp) {
  auto const us = std::chrono::time_point_cast<std::chrono::microseconds>(tp);
  auto const days = std::chrono::floor<std::chrono::days>(us);
  auto const ymd = std::chrono::year_month_day{days};
  auto const tod = std::chrono::hh_mm_ss{us - days};
  WritePaddedUnsigned(writer,
                      static_cast<unsigned>(static_cast<int>(ymd.year())), 4);
  writer.write('-');
  WritePaddedUnsigned(writer, static_cast<unsigned>(ymd.month()), kTwoDigitWidth);
  writer.write('-');
  WritePaddedUnsigned(writer, static_cast<unsigned>(ymd.day()), kTwoDigitWidth);
  writer.write(' ');
  WriteTimeOfDayMicros(writer, tod);
}

template <typename Writer>
void WriteChronoTimeOfDay(Writer& writer,
                          std::chrono::system_clock::time_point tp) {
  auto const us = std::chrono::time_point_cast<std::chrono::microseconds>(tp);
  auto const days = std::chrono::floor<std::chrono::days>(us);
  auto const tod = std::chrono::hh_mm_ss{us - days};
  WriteTimeOfDayMicros(writer, tod);
}

template <typename Writer, typename Rep, typename Period>
  requires SupportedDuration<Rep, Period>
void WriteChronoDuration(Writer& writer,
                         std::chrono::duration<Rep, Period> value) {
  using Magnitude = std::make_unsigned_t<Rep>;
  auto negative = false;
  auto abs = Magnitude{};
  if constexpr (std::is_signed_v<Rep>) {
    auto const count = value.count();
    negative = count < 0;
    abs = negative ? Magnitude{0} - static_cast<Magnitude>(count)
                   : static_cast<Magnitude>(count);
  } else {
    abs = value.count();
  }
  auto hours = Magnitude{};
  auto minutes = Magnitude{};
  auto seconds = Magnitude{};
  auto millis = unsigned{};
  auto micros = unsigned{};
  auto nanos = unsigned{};

  if constexpr (std::ratio_equal_v<Period, std::nano>) {
    nanos = static_cast<unsigned>(TakeRemainder<kSubsecondBase>(abs));
  }
  if constexpr (std::ratio_equal_v<Period, std::nano> ||
                 std::ratio_equal_v<Period, std::micro>) {
    micros = static_cast<unsigned>(TakeRemainder<kSubsecondBase>(abs));
  }
  if constexpr (std::ratio_equal_v<Period, std::nano> ||
                 std::ratio_equal_v<Period, std::micro> ||
                 std::ratio_equal_v<Period, std::milli>) {
    millis = static_cast<unsigned>(TakeRemainder<kSubsecondBase>(abs));
  }
  if constexpr (!std::ratio_equal_v<Period, HoursPeriod> &&
                !std::ratio_equal_v<Period, MinutesPeriod>) {
    seconds = TakeRemainder<kSecondsPerMinute>(abs);
  }
  if constexpr (!std::ratio_equal_v<Period, HoursPeriod>) {
    minutes = TakeRemainder<kMinutesPerHour>(abs);
  }
  hours = abs;

  if (negative) {
    writer.write('-');
  }
  WriteHourMinuteSecond(writer, hours, minutes, seconds, false);
  if constexpr (std::ratio_equal_v<Period, std::nano> ||
                std::ratio_equal_v<Period, std::micro> ||
                std::ratio_equal_v<Period, std::milli>) {
    writer.write('.');
    WritePaddedUnsigned(writer, millis, kSubsecondChunkWidth);
  }
  if constexpr (std::ratio_equal_v<Period, std::nano> ||
                std::ratio_equal_v<Period, std::micro>) {
    WritePaddedUnsigned(writer, micros, kSubsecondChunkWidth);
  }
  if constexpr (std::ratio_equal_v<Period, std::nano>) {
    WritePaddedUnsigned(writer, nanos, kSubsecondChunkWidth);
  }
}

}  // namespace format_internal

// Formats std::chrono::system_clock::time_point only. Supported values are
// non-negative offsets since the Unix epoch. Output precision is microseconds;
// higher precision inputs are truncated, not rounded. The {:time} option prints
// only the time of day, while all other options print the full date and time.
template <typename Duration>
struct Formatter<std::chrono::time_point<std::chrono::system_clock, Duration>> {
  template <typename Writer>
  void Format(
      std::chrono::time_point<std::chrono::system_clock, Duration> const& value,
      FormatContext<Writer>& ctx) const {
    auto const us = std::chrono::duration_cast<std::chrono::microseconds>(
        value.time_since_epoch());
    auto const tp = std::chrono::system_clock::time_point{us};
    if (ctx.options == "time") {
      format_internal::WriteChronoTimeOfDay(ctx.out(), tp);
    } else {
      format_internal::WriteChronoDateTime(ctx.out(), tp);
    }
  }
};

template <typename Rep, typename Period>
  requires format_internal::SupportedDuration<Rep, Period>
struct Formatter<std::chrono::duration<Rep, Period>> {
  template <typename Writer>
  void Format(std::chrono::duration<Rep, Period> const& value,
              FormatContext<Writer>& ctx) const {
    format_internal::WriteChronoDuration(ctx.out(), value);
  }
};

}  // namespace ae

#endif  // AETHER_MISCPP_FORMAT_FORMAT_TIME_H_

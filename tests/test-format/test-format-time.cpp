/*
 * Copyright 2024 Aethernet Inc.
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

#include <unity.h>

#include <chrono>
#include <cstdint>
#include <list>
#include <limits>

#include "aether-miscpp/format/format.h"

namespace ae::test_format_time {
using TimePoint =
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;
using SteadyTimePoint =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;
using UnsupportedSteadyTimePoint =
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>;

template <typename T>
concept HasStringFormatter = requires(ae::Formatter<T> formatter, T const& value,
                                       ae::FormatContext<
                                           ae::format_internal::StringWriter>& ctx) {
  formatter.Format(value, ctx);
};

static_assert(HasStringFormatter<TimePoint>);
static_assert(!HasStringFormatter<UnsupportedSteadyTimePoint>);
static_assert(!HasStringFormatter<std::chrono::days>);
static_assert(!HasStringFormatter<std::chrono::weeks>);
static_assert(!HasStringFormatter<std::chrono::duration<int, std::pico>>);
static_assert(!HasStringFormatter<std::chrono::duration<int, std::ratio<2>>>);
static_assert(HasStringFormatter<std::chrono::duration<int, std::ratio<3600>>>);

}  // namespace ae::test_format_time

template <>
struct ae::Formatter<ae::test_format_time::SteadyTimePoint> {
  template <typename TStream>
  void Format(ae::test_format_time::SteadyTimePoint const&,
              ae::FormatContext<TStream>& ctx) const {
    ctx.out().write("steady");
  }
};

namespace ae::test_format_time {

void test_FormatTimePoint() {
  auto tp =
      std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>{
          std::chrono::microseconds{1234567}};
  auto tp_str = Format("{}", tp);
  TEST_ASSERT_EQUAL_STRING("1970-01-01 00:00:01.234567", tp_str.data());
  auto tp_time_str = Format("{:time}", tp);
  TEST_ASSERT_EQUAL_STRING("00:00:01.234567", tp_time_str.data());
  auto tp_unknown_str = Format("{time}", tp);
  TEST_ASSERT_EQUAL_STRING("1970-01-01 00:00:01.234567", tp_unknown_str.data());

  auto tp2 = TimePoint{std::chrono::milliseconds{9913675}};
  auto tp2_str = Format("[{}]", tp2);
  TEST_ASSERT_EQUAL_STRING("[1970-01-01 02:45:13.675000]", tp2_str.data());
  auto tp2_time_str = Format("{:time}", tp2);
  TEST_ASSERT_EQUAL_STRING("02:45:13.675000", tp2_time_str.data());

  auto steady_str = Format("{}", SteadyTimePoint{});
  TEST_ASSERT_EQUAL_STRING("steady", steady_str.data());
}

void test_FormatNanosecondTimePointTruncatesToMicros() {
  auto tp =
      std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>{
          std::chrono::nanoseconds{1234567890}};

  auto tp_str = Format("{}", tp);
  TEST_ASSERT_EQUAL_STRING("1970-01-01 00:00:01.234567", tp_str.data());

  auto tp_time_str = Format("{:time}", tp);
  TEST_ASSERT_EQUAL_STRING("00:00:01.234567", tp_time_str.data());
}

void test_FormatListOfTimePoint() {
  auto tp_list = std::list{TimePoint{std::chrono::milliseconds{13675}},
                           TimePoint{std::chrono::milliseconds{14000}},
                           TimePoint{std::chrono::milliseconds{15888}}};
  auto tp_list_str = Format("{}", tp_list);
  TEST_ASSERT_EQUAL_STRING("1970-01-01 00:00:13.675000, 1970-01-01 00:00:14.000000, 1970-01-01 00:00:15.888000",
                           tp_list_str.data());
}

void test_Duration() {
  TEST_ASSERT_EQUAL_STRING("1:00:00", Format("{}", std::chrono::hours{1}).data());
  TEST_ASSERT_EQUAL_STRING("15:00:00", Format("{}", std::chrono::hours{15}).data());
  TEST_ASSERT_EQUAL_STRING("25:00:00", Format("{}", std::chrono::hours{25}).data());
  TEST_ASSERT_EQUAL_STRING("100:00:00", Format("{}", std::chrono::hours{100}).data());
  TEST_ASSERT_EQUAL_STRING("1:01:00", Format("{}", std::chrono::minutes{61}).data());
  TEST_ASSERT_EQUAL_STRING("25:01:00", Format("{}", std::chrono::minutes{1501}).data());
  TEST_ASSERT_EQUAL_STRING("0:01:01", Format("{}", std::chrono::seconds{61}).data());
  TEST_ASSERT_EQUAL_STRING("25:01:01", Format("{}", std::chrono::seconds{90061}).data());
  TEST_ASSERT_EQUAL_STRING("0:00:00.001", Format("{}", std::chrono::milliseconds{1}).data());
  TEST_ASSERT_EQUAL_STRING("25:01:01.002", Format("{}", std::chrono::milliseconds{90061002}).data());
  auto d = std::chrono::microseconds{-3723004005LL};
  auto s = Format("{}", d);
  TEST_ASSERT_EQUAL_STRING("-1:02:03.004005", s.data());
  TEST_ASSERT_EQUAL_STRING("25:01:01.002003004",
                           Format("{}", std::chrono::nanoseconds{90061002003004LL}).data());

  using TwoHourDuration = std::chrono::duration<int, std::ratio<3600>>;
  TEST_ASSERT_EQUAL_STRING("25:00:00", Format("{}", TwoHourDuration{25}).data());

  auto min_s = Format("{}", std::chrono::microseconds::min());
  TEST_ASSERT_EQUAL_STRING("-2562047788:00:54.775808", min_s.data());

  using Int8Millis = std::chrono::duration<int8_t, std::milli>;
  using Int16Micros = std::chrono::duration<int16_t, std::micro>;
  using Int16Nanos = std::chrono::duration<int16_t, std::nano>;
  using Int32Micros = std::chrono::duration<int32_t, std::micro>;
  using Int32Nanos = std::chrono::duration<int32_t, std::nano>;
  TEST_ASSERT_EQUAL_STRING("0:00:00.127", Format("{}", Int8Millis{127}).data());
  TEST_ASSERT_EQUAL_STRING("-0:00:00.128",
                           Format("{}", Int8Millis{std::numeric_limits<int8_t>::min()}).data());
  TEST_ASSERT_EQUAL_STRING("0:00:00.032767",
                           Format("{}", Int16Micros{std::numeric_limits<int16_t>::max()}).data());
  TEST_ASSERT_EQUAL_STRING("-0:00:00.032768",
                           Format("{}", Int16Micros{std::numeric_limits<int16_t>::min()}).data());
  TEST_ASSERT_EQUAL_STRING("0:00:00.000032767",
                           Format("{}", Int16Nanos{std::numeric_limits<int16_t>::max()}).data());
  TEST_ASSERT_EQUAL_STRING("-0:00:00.000032768",
                           Format("{}", Int16Nanos{std::numeric_limits<int16_t>::min()}).data());
  TEST_ASSERT_EQUAL_STRING("0:00:02.147483647",
                           Format("{}", Int32Nanos{std::numeric_limits<int32_t>::max()}).data());
  TEST_ASSERT_EQUAL_STRING("-0:00:02.147483648",
                           Format("{}", Int32Nanos{std::numeric_limits<int32_t>::min()}).data());
  TEST_ASSERT_EQUAL_STRING("0:35:47.483647",
                           Format("{}", Int32Micros{std::numeric_limits<int32_t>::max()}).data());
  TEST_ASSERT_EQUAL_STRING("-0:35:47.483648",
                           Format("{}", Int32Micros{std::numeric_limits<int32_t>::min()}).data());
}
}  // namespace ae::test_format_time

int test_format_time() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_format_time::test_FormatTimePoint);
  RUN_TEST(ae::test_format_time::test_FormatNanosecondTimePointTruncatesToMicros);
  RUN_TEST(ae::test_format_time::test_FormatListOfTimePoint);
  RUN_TEST(ae::test_format_time::test_Duration);
  return UNITY_END();
}

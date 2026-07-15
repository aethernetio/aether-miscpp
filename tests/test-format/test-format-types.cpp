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

#include <array>
#include <cstdint>
#include <iterator>
#include <list>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "aether-miscpp/format/format.h"

namespace ae::test_format_types {
struct CustomType {
  int value;
};

struct UnsupportedType {};

using TestWriter = ae::format_internal::StringWriter;

static_assert(
    ae::format_internal::HasFormatterFor<std::optional<int>, TestWriter>);
static_assert(
    !ae::format_internal::HasFormatterFor<std::optional<UnsupportedType>,
                                          TestWriter>);
static_assert(ae::format_internal::HasFormatterFor<std::vector<int>,
                                                   TestWriter>);
static_assert(ae::format_internal::HasFormatterFor<std::vector<std::vector<int>>,
                                                   TestWriter>);
static_assert(!ae::format_internal::HasFormatterFor<std::vector<UnsupportedType>,
                                                    TestWriter>);

struct ByteProxy {
  operator std::uint8_t() const { return value; }

  ByteProxy& operator=(std::uint8_t) {
    *mutated = true;
    return *this;
  }

  std::uint8_t value{};
  bool* mutated{};
};

struct ProxyByteIterator {
  using difference_type = std::ptrdiff_t;
  using value_type = std::uint8_t;
  using iterator_concept = std::input_iterator_tag;

  ByteProxy operator*() const { return ByteProxy{data[index], mutated}; }
  ProxyByteIterator& operator++() {
    ++index;
    return *this;
  }
  void operator++(int) { ++(*this); }

  friend bool operator==(ProxyByteIterator const& lhs,
                         ProxyByteIterator const& rhs) {
    return lhs.index == rhs.index;
  }

  std::uint8_t const* data{};
  std::size_t index{};
  bool* mutated{};
};

struct ProxyByteRange {
  using value_type = std::uint8_t;

  ProxyByteIterator begin() const {
    return ProxyByteIterator{data.data(), 0, mutated};
  }
  ProxyByteIterator end() const {
    return ProxyByteIterator{data.data(), data.size(), mutated};
  }

  std::array<std::uint8_t, 3> data{};
  bool* mutated{};
};

}  // namespace ae::test_format_types

template <>
struct ae::Formatter<ae::test_format_types::CustomType> {
  template <typename TStream>
  void Format(ae::test_format_types::CustomType const& value,
              ae::FormatContext<TStream>& ctx) const {
    if (value.value == 43) {
      TEST_ASSERT_EQUAL_STRING("custom", std::string{ctx.options}.c_str());
    } else {
      TEST_ASSERT_EQUAL_UINT32(0, static_cast<unsigned>(ctx.options.size()));
    }
    auto nested = ae::FormatScheme{"custom:{}"};
    ae::FormatTo(ctx.out(), nested, value.value);
  }
};

namespace ae::test_format_types {
void test_FormatNumbers() {
  auto int8 = Format("{}", static_cast<int>(std::int8_t{42}));
  auto int16 = Format("{}", std::int16_t{42});
  auto int32 = Format("{}", std::int32_t{42});
  auto int64 = Format("{}", std::int64_t{42});
  auto negative = Format("{}", std::int64_t{-42});
  auto int64_min = Format("{}", std::numeric_limits<std::int64_t>::min());

  TEST_ASSERT_EQUAL_STRING("42", int8.data());
  TEST_ASSERT_EQUAL_STRING("42", int16.data());
  TEST_ASSERT_EQUAL_STRING("42", int32.data());
  TEST_ASSERT_EQUAL_STRING("42", int64.data());
  TEST_ASSERT_EQUAL_STRING("-42", negative.data());
  TEST_ASSERT_EQUAL_STRING("-9223372036854775808", int64_min.data());

  auto uint8 = Format("{}", static_cast<int>(std::uint8_t{42}));
  auto uint16 = Format("{}", std::uint16_t{42});
  auto uint32 = Format("{}", std::uint32_t{42});
  auto uint64 = Format("{}", std::uint64_t{42});
  auto uint64_max = Format("{}", std::numeric_limits<std::uint64_t>::max());

  TEST_ASSERT_EQUAL_STRING("42", uint8.data());
  TEST_ASSERT_EQUAL_STRING("42", uint16.data());
  TEST_ASSERT_EQUAL_STRING("42", uint32.data());
  TEST_ASSERT_EQUAL_STRING("42", uint64.data());
  TEST_ASSERT_EQUAL_STRING("18446744073709551615", uint64_max.data());
}

void test_FormatFloats() {
  auto float_42 = Format("{}", 4.2F);
  auto float_354 = Format("{}", 35.4F);
  auto double_42 = Format("{}", 4.2);
  auto double_354 = Format("{}", 35.4);

  TEST_ASSERT_EQUAL_STRING("4.2", float_42.data());
  TEST_ASSERT_EQUAL_STRING("35.4", float_354.data());
  TEST_ASSERT_EQUAL_STRING("4.2", double_42.data());
  TEST_ASSERT_EQUAL_STRING("35.4", double_354.data());
}

enum class Int8EnumType : std::int8_t {
  kOne = 1,
  kFortyTwo = 42,
};

enum class Int16EnumType : std::int16_t {
  kOne = 1,
  kFortyTwoAndALittle = 4200,
};

void test_FormatEnums() {
  auto one_8 = Format("{}", Int8EnumType::kOne);
  auto forty_two_8 = Format("{}", Int8EnumType::kFortyTwo);

  TEST_ASSERT_EQUAL_STRING("1", one_8.data());
  TEST_ASSERT_EQUAL_STRING("42", forty_two_8.data());

  auto one_16 = Format("{}", Int16EnumType::kOne);
  auto forty_two_16 = Format("{}", Int16EnumType::kFortyTwoAndALittle);

  TEST_ASSERT_EQUAL_STRING("1", one_16.data());
  TEST_ASSERT_EQUAL_STRING("4200", forty_two_16.data());
}

void test_FormatStrings() {
  auto hello = Format("{}", std::string_view{"hello"});
  auto world = Format("{}", std::string{"world"});

  TEST_ASSERT_EQUAL_STRING("hello", hello.data());
  TEST_ASSERT_EQUAL_STRING("world", world.data());
}

void test_Containers() {
  std::vector<std::int8_t> vec_data = {0x7f, 0x01, 0x42};
  auto vec_data_str = Format("{}", vec_data);
  TEST_ASSERT_EQUAL_STRING("127, 1, 66", vec_data_str.data());

  std::vector<std::uint8_t> bytes = {0x7f, 0x01, 0x42};
  auto bytes_str = Format("{}", bytes);
  TEST_ASSERT_EQUAL_STRING("0x7f0142", bytes_str.data());

  std::vector<std::string_view> vec_messages = {"hello", "beautiful", "world"};
  auto vec_messages_str = Format("[{}]", vec_messages);
  TEST_ASSERT_EQUAL_STRING("[hello, beautiful, world]",
                           vec_messages_str.data());

  std::list<std::uint8_t> list_data = {0xff, 0x01, 0x42};
  auto list_data_str = Format("{}", list_data);
  TEST_ASSERT_EQUAL_STRING("0xff0142", list_data_str.data());

  std::list<Int8EnumType> list_enum = {
      Int8EnumType::kOne, Int8EnumType::kFortyTwo, Int8EnumType::kOne};
  auto list_enum_str = Format("[{}]", list_enum);
  TEST_ASSERT_EQUAL_STRING("[1, 42, 1]", list_enum_str.data());

  auto arr_data = std::array{0xff, 0x01, 0x42};
  auto arr_data_str = Format("{}", arr_data);
  TEST_ASSERT_EQUAL_STRING("255, 1, 66", arr_data_str.data());

  std::vector<std::vector<int>> nested{{1, 2}, {3, 4}};
  auto nested_str = Format("{}", nested);
  TEST_ASSERT_EQUAL_STRING("[1, 2], [3, 4]", nested_str.data());

  auto arr_floats = std::array{25.5F, 4.2F, 35.4F};
  auto arr_floats_str = Format("{}", arr_floats);
  TEST_ASSERT_EQUAL_STRING("25.5, 4.2, 35.4", arr_floats_str.data());

  auto mutated = false;
  auto proxy_bytes = ProxyByteRange{{0x7f, 0x01, 0x42}, &mutated};
  auto proxy_bytes_str = Format("{}", proxy_bytes);
  TEST_ASSERT_EQUAL_STRING("0x7f0142", proxy_bytes_str.data());
  TEST_ASSERT_FALSE(mutated);
}

void test_Optional() {
  auto opt_value = std::optional{42};
  auto opt_value_str = Format("{}", opt_value);
  TEST_ASSERT_EQUAL_STRING("42", opt_value_str.data());

  auto opt_no_value = std::optional<std::string>{};
  auto opt_no_value_str = Format("{}", opt_no_value);
  TEST_ASSERT_EQUAL_STRING("nullopt", opt_no_value_str.data());
}

void test_CustomFormatterAndEnums() {
  auto out = Format("{}", CustomType{42});
  TEST_ASSERT_EQUAL_STRING("custom:42", out.data());

  auto out_with_options = Format("{:custom}", CustomType{43});
  TEST_ASSERT_EQUAL_STRING("custom:43", out_with_options.data());
}

}  // namespace ae::test_format_types

int test_format_types() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_format_types::test_FormatNumbers);
  RUN_TEST(ae::test_format_types::test_FormatFloats);
  RUN_TEST(ae::test_format_types::test_FormatEnums);
  RUN_TEST(ae::test_format_types::test_FormatStrings);
  RUN_TEST(ae::test_format_types::test_Containers);
  RUN_TEST(ae::test_format_types::test_Optional);
  RUN_TEST(ae::test_format_types::test_CustomFormatterAndEnums);
  return UNITY_END();
}

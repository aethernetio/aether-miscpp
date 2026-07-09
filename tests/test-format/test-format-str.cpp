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
#include <sstream>
#include <string>
#include <string_view>

#include "aether-miscpp/format/format.h"

namespace ae::test_format_str {

static_assert(FormatScheme::kMaxFormatParts == 21);

constexpr auto kConstexprScheme = FormatScheme{"{} {{}}"};
static_assert(kConstexprScheme.source.size() == 7);
static_assert(kConstexprScheme.part_count == 4);
static_assert(!kConstexprScheme.overflow);
static_assert(kConstexprScheme.parts[0].placeholder);
static_assert(!kConstexprScheme.parts[1].placeholder);

constexpr auto kEnvFormatScheme = FormatScheme{
    "Platform:{}\n"
    "Compiler:{}\n"
    "Compiler version:{}\n"
    "Library version:{}\n"
    "Api version:{}\n"
    "CPU arch:{}\n"
    "Endianness:{}\n"
    "UTMid:{}\n"};
constexpr auto kEnvPlaceholderOffsets =
    std::array<std::size_t, 8>{9, 21, 41, 60, 75, 87, 101, 110};

constexpr bool EnvPlaceholdersMatch() {
  for (std::size_t i = 0; i < kEnvPlaceholderOffsets.size(); ++i) {
    auto const part = kEnvFormatScheme.parts[(i * 2) + 1];
    if (!part.placeholder || part.offset != kEnvPlaceholderOffsets[i] ||
        part.size != 2 || kEnvFormatScheme.source[part.offset] != '{' ||
        kEnvFormatScheme.source[part.offset + 1] != '}') {
      return false;
    }
  }
  return true;
}

static_assert(kEnvFormatScheme.source.size() == 113);
static_assert(kEnvFormatScheme.part_count == 17);
static_assert(!kEnvFormatScheme.overflow);
static_assert(EnvPlaceholdersMatch());

void test_BracketInTheBegin() {
  auto str1 = Format("{ kek");
  TEST_ASSERT_EQUAL_STRING("{ kek", str1.c_str());
  auto str2 = Format("{} kek");
  TEST_ASSERT_EQUAL_STRING("{} kek", str2.c_str());

  auto str3 = Format("} kek {");
  TEST_ASSERT_EQUAL_STRING("} kek {", str3.c_str());
}

void test_BracketInTheEnd() {
  auto str1 = Format("kek {");
  TEST_ASSERT_EQUAL_STRING("kek {", str1.c_str());
  auto str2 = Format("kek {}");
  TEST_ASSERT_EQUAL_STRING("kek {}", str2.c_str());
}

void test_BracketEscape() {
  auto str1 = Format("kek {{}}");
  TEST_ASSERT_EQUAL_STRING("kek {}", str1.c_str());

  auto str2 = Format("{} kek {{}}");
  TEST_ASSERT_EQUAL_STRING("{} kek {}", str2.c_str());

  auto str3 = Format("{} kek {{{}}}", 12, 42);
  TEST_ASSERT_EQUAL_STRING("12 kek {42}", str3.c_str());

  auto str4 = Format("{{}}");
  TEST_ASSERT_EQUAL_STRING("{}", str4.c_str());

  auto str5 = Format("{{}");
  TEST_ASSERT_EQUAL_STRING("{}", str5.c_str());

  auto str6 = Format("{}}", 1);
  TEST_ASSERT_EQUAL_STRING("1}", str6.c_str());

  auto str7 = Format("{ {{");
  TEST_ASSERT_EQUAL_STRING("{ {{", str7.c_str());

  auto str8 = Format("}} {");
  TEST_ASSERT_EQUAL_STRING("} {", str8.c_str());

  auto str9 = Format("x { y {{ z", 123);
  TEST_ASSERT_EQUAL_STRING("x { y {{ z", str9.c_str());
}

void test_MissingArgsAndLiteralNul() {
  auto str = Format(std::string_view{"a{b}c"});
  TEST_ASSERT_EQUAL_UINT32(5, static_cast<unsigned>(str.size()));
  TEST_ASSERT_EQUAL_STRING("a{b}c", str.c_str());

  auto option_missing_arg = Format("{:time}");
  TEST_ASSERT_EQUAL_STRING("{:time}", option_missing_arg.c_str());

  auto lit = Format("abc\0def");
  TEST_ASSERT_EQUAL_UINT32(7, static_cast<unsigned>(lit.size()));
  TEST_ASSERT_EQUAL_CHAR('a', lit[0]);
  TEST_ASSERT_EQUAL_CHAR('b', lit[1]);
  TEST_ASSERT_EQUAL_CHAR('c', lit[2]);
  TEST_ASSERT_EQUAL_CHAR('\0', lit[3]);
  TEST_ASSERT_EQUAL_CHAR('d', lit[4]);
  TEST_ASSERT_EQUAL_CHAR('e', lit[5]);
  TEST_ASSERT_EQUAL_CHAR('f', lit[6]);
}

void test_FormatTo() {
  std::string out{"x"};
  FormatTo(out, "-{}-", 42);
  TEST_ASSERT_EQUAL_STRING("x-42-", out.c_str());

  std::ostringstream oss;
  FormatTo(oss, "{}:{}", 1, 2);
  TEST_ASSERT_EQUAL_STRING("1:2", oss.str().c_str());
}

void test_FormatSchemeRuntimeAndOverflow() {
  constexpr auto scheme = FormatScheme{"{} {{}}"};
  static_assert(scheme.source.size() == 7);
  TEST_ASSERT_EQUAL_STRING("1 {}", Format(scheme, 1).c_str());

  auto runtime_format = std::string{"{}:{}"};
  auto runtime = Format(std::string_view{runtime_format}, 1, 2);
  TEST_ASSERT_EQUAL_STRING("1:2", runtime.c_str());

  auto many = std::string{};
  for (auto i = 0; i < 70; ++i) {
    many += "{}";
  }
  auto overflow = Format(std::string_view{many}, 1, 2, 3);
  auto expected = many + " OVERFLOW";
  TEST_ASSERT_EQUAL_STRING(expected.c_str(), overflow.c_str());
}

void test_FormatSchemeTenPlaceholdersFit() {
  constexpr auto scheme = FormatScheme{"a{}b{}c{}d{}e{}f{}g{}h{}i{}j{}k"};
  static_assert(scheme.part_count == 21);
  static_assert(!scheme.overflow);

  auto result = Format(scheme, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
  TEST_ASSERT_EQUAL_STRING("a1b2c3d4e5f6g7h8i9j10k", result.c_str());
}

void test_FormatSchemeElevenPlaceholdersOverflow() {
  constexpr auto scheme = FormatScheme{"a{}b{}c{}d{}e{}f{}g{}h{}i{}j{}k{}l"};
  static_assert(scheme.overflow);

  auto result = Format(scheme, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
  TEST_ASSERT_EQUAL_STRING("a{}b{}c{}d{}e{}f{}g{}h{}i{}j{}k{}l OVERFLOW",
                           result.c_str());
}

void test_FormatToRuntimeFormatSource() {
  auto out = std::string{"prefix:"};
  auto format = std::string{"{}"};
  auto scheme = FormatScheme{std::string_view{format}};
  FormatTo(out, scheme, std::string(128, 'x'));
  auto expected = std::string{"prefix:"} + std::string(128, 'x');
  TEST_ASSERT_EQUAL_STRING(expected.c_str(), out.c_str());
}
}  // namespace ae::test_format_str

int test_format_str() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_format_str::test_BracketInTheBegin);
  RUN_TEST(ae::test_format_str::test_BracketInTheEnd);
  RUN_TEST(ae::test_format_str::test_BracketEscape);
  RUN_TEST(ae::test_format_str::test_MissingArgsAndLiteralNul);
  RUN_TEST(ae::test_format_str::test_FormatTo);
  RUN_TEST(ae::test_format_str::test_FormatSchemeRuntimeAndOverflow);
  RUN_TEST(ae::test_format_str::test_FormatSchemeTenPlaceholdersFit);
  RUN_TEST(ae::test_format_str::test_FormatSchemeElevenPlaceholdersOverflow);
  RUN_TEST(ae::test_format_str::test_FormatToRuntimeFormatSource);
  return UNITY_END();
}

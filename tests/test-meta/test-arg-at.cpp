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

#include <unity.h>

#include "aether-miscpp/meta/arg_at.h"

namespace ae::test_arg_at {

template <auto... Args>
inline void testTemplateArgs() {
  TEST_ASSERT_EQUAL(10, (ae::ArgAt_v<0, Args...>));
  TEST_ASSERT_EQUAL_FLOAT(3.14F, (ae::ArgAt_v<1, Args...>));
  TEST_ASSERT_EQUAL(true, (ae::ArgAt_v<2, Args...>));
}

void test_VarAt() {
  constexpr int x = 42;
  constexpr double y = 3.14;
  constexpr bool z = true;

  static_assert(ae::VarAt<0>(x, y, z) == 42);
  static_assert(ae::VarAt<1>(x, y, z) == 3.14);
  static_assert(ae::VarAt<2>(x, y, z));

  int a = 10;
  float b = 3.14F;
  bool c = true;

  TEST_ASSERT_EQUAL(10, ae::VarAt<0>(a, b, c));
  TEST_ASSERT_EQUAL_FLOAT(3.14F, ae::VarAt<1>(a, b, c));
  TEST_ASSERT_EQUAL(true, ae::VarAt<2>(a, b, c));
}

void test_ArgAt() {
  constexpr int x = 42;
  constexpr double y = 3.14;
  constexpr bool z = true;

  static_assert(ae::ArgAt_v<0, x, y, z> == 42);
  static_assert(ae::ArgAt_v<1, x, y, z> == 3.14);
  static_assert(ae::ArgAt_v<2, x, y, z>);

  testTemplateArgs<10, 3.14F, true>();
}

}  // namespace ae::test_arg_at

int test_arg_at() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_arg_at::test_VarAt);
  RUN_TEST(ae::test_arg_at::test_ArgAt);
  return UNITY_END();
}

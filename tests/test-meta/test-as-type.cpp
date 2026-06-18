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

#include "aether-miscpp/meta/as_type.h"

namespace ae::test_as_type {

struct Base {};
struct Derived : Base {};

void test_AsType() {
  Derived d;
  decltype(auto) b1 = ae::as_type<Base>(d);
  static_assert(std::is_same_v<Base&, decltype(b1)>);

  decltype(auto) b2 = ae::as_type<Base>(std::as_const(d));
  static_assert(std::is_same_v<Base const&, decltype(b2)>);

  decltype(auto) b3 = ae::as_type<Base>(std::move(d));
  static_assert(std::is_same_v<Base&&, decltype(b3)>);

  TEST_PASS();
}

}  // namespace ae::test_as_type

int test_as_type() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_as_type::test_AsType);
  return UNITY_END();
}

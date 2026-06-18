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

#include "aether-miscpp/meta/tag_invoke.h"

namespace ae::test_tag_invoke {

inline constexpr struct TestCpo {
  void operator()(ae::TagInvocable<TestCpo> auto& t) const {
    ae::tag_invoke(*this, t);
  }
} test_cpo{};

inline constexpr struct TestGetInt {
  int operator()(ae::TagInvocable<TestGetInt> auto& t) const {
    return ae::tag_invoke(*this, t);
  }
} test_get_int{};

struct Foo {
  bool& invoked;

  friend void tag_invoke(ae::tag_t<test_cpo>, Foo& foo) noexcept {
    foo.invoked = true;
  }
};

struct Bar {
  int value;

  friend int tag_invoke(ae::tag_t<test_get_int>, Bar const& bar) noexcept {
    return bar.value;
  }
};

void test_TagInvoke() {
  using TestCpoType = ae::tag_t<test_cpo>;
  static_assert(std::is_same_v<TestCpo, TestCpoType>);
  using TestGetIntType = ae::tag_t<test_get_int>;
  static_assert(std::is_same_v<TestGetInt, TestGetIntType>);

  using foo_res = ae::tag_invoke_result_t<ae::tag_t<test_cpo>, Foo&>;
  static_assert(std::is_void_v<foo_res>);
  static_assert(ae::TagInvocable_v<ae::tag_t<test_cpo>, Foo&>);
  static_assert(!ae::TagInvocable_v<ae::tag_t<test_get_int>, Foo&>);

  using bar_res = ae::tag_invoke_result_t<ae::tag_t<test_get_int>, Bar const&>;
  static_assert(std::is_same_v<int, bar_res>);
  static_assert(ae::TagInvocable_v<ae::tag_t<test_get_int>, Bar const&>);
  static_assert(!ae::TagInvocable_v<ae::tag_t<test_cpo>, Bar const&>);

  static_assert(ae::TagInvocable<Foo, ae::tag_t<test_cpo>>);
  static_assert(!ae::TagInvocable<Foo, ae::tag_t<test_get_int>>);
  static_assert(ae::TagInvocable<Bar, ae::tag_t<test_get_int>>);
  static_assert(!ae::TagInvocable<Bar, ae::tag_t<test_cpo>>);

  bool invoked{};
  Foo foo{invoked};
  test_cpo(foo);
  TEST_ASSERT_TRUE(invoked);

  Bar bar{42};
  auto value = test_get_int(bar);
  TEST_ASSERT_EQUAL(42, value);
}

}  // namespace ae::test_tag_invoke

int test_tag_invoke() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_tag_invoke::test_TagInvoke);
  return UNITY_END();
}

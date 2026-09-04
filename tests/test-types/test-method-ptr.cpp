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

#include <type_traits>

#include <unity.h>

#include "aether-miscpp/types/method_ptr.h"

namespace ae::test_method_ptr {

int FreeFunction(int value) { return value + 1; }

struct Worker {
  int Add(int value) {
    total += value;
    return total;
  }

  int AddConst(int value) const { return total + value; }

  int total{};
};

using FreeFunctionWrapper = MethodPtr<&FreeFunction>;
using NonConstMemberWrapper = MethodPtr<&Worker::Add>;
using ConstMemberWrapper = MethodPtr<&Worker::AddConst>;

// MethodPtr wrappers preserve aggregate trivial representation and recognize
// only their plain types.
static_assert(std::is_aggregate_v<FreeFunctionWrapper>);
static_assert(std::is_trivial_v<FreeFunctionWrapper>);
static_assert(std::is_aggregate_v<NonConstMemberWrapper>);
static_assert(std::is_trivial_v<NonConstMemberWrapper>);
static_assert(std::is_aggregate_v<ConstMemberWrapper>);
static_assert(std::is_trivial_v<ConstMemberWrapper>);
static_assert(MethodPtrType<NonConstMemberWrapper>);

// Verifies free, non-const member, and const member wrappers invoke targets.
void test_MethodPtrWrappers() {
  auto free_function = FreeFunctionWrapper{};
  TEST_ASSERT_EQUAL_INT(2, free_function(1));

  auto worker = Worker{};
  auto non_const_member = NonConstMemberWrapper{&worker};
  auto const_member = ConstMemberWrapper{&worker};

  TEST_ASSERT_EQUAL_INT(3, non_const_member(3));
  TEST_ASSERT_EQUAL_INT(7, const_member(4));
}

}  // namespace ae::test_method_ptr

int test_method_ptr() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_method_ptr::test_MethodPtrWrappers);
  return UNITY_END();
}

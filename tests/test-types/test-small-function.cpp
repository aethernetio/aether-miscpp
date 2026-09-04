/*
 * Copyright 2025 Aethernet Inc.
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

#include <concepts>
#include <vector>

#include "aether-miscpp/types/small_function.h"

namespace ae::test_small_function {
void test_CallALambdas() {
  using Func = SmallFunction<void(int)>;
  auto f1_called = false;
  // Create inplace
  auto f1 = Func{[&](int x) {
    f1_called = true;
    TEST_ASSERT_EQUAL(1, x);
  }};

  f1(1);
  TEST_ASSERT_TRUE(f1_called);

  auto f2_called = false;
  auto l2 = [&](int x) {
    f2_called = true;
    TEST_ASSERT_EQUAL(2, x);
  };
  // Create by move
  auto f2 = Func{std::move(l2)};

  f2(2);
  TEST_ASSERT_TRUE(f2_called);

  auto f3_called = false;
  auto l3 = [&](int x) {
    f3_called = true;
    TEST_ASSERT_EQUAL(3, x);
  };
  // Create by copy
  auto f3 = Func{l3};

  f3(3);
  TEST_ASSERT_TRUE(f3_called);
}

// Verifies a const wrapper can invoke a mutable callable.
void test_ConstWrapperCallsMutableCallable() {
  auto invoke_count = 0;
  const SmallFunction<void()> function{[count = 0, &invoke_count]() mutable {
    count++;
    invoke_count += count;
  }};

  function();
  function();

  TEST_ASSERT_EQUAL(3, invoke_count);
}

void test_FunctionsInVector() {
  using Func = SmallFunction<void(int)>;
  std::vector<Func> funcs;
  std::size_t invoke_count = 0;
  for (int i = 0; i < 255; i++) {
    // each emplace back should grow the vector, make all the elements remove to
    // new data buffer
    funcs.emplace_back([&invoke_count, i](int x) {
      invoke_count++;
      TEST_ASSERT_EQUAL(i, x);
    });
    // make it grow each time
    funcs.shrink_to_fit();
  }

  for (std::size_t i = 0; i < 255; i++) {
    funcs[i](static_cast<int>(i));
  }

  TEST_ASSERT_EQUAL(255, invoke_count);
}

struct DestructableCallable {
  static inline int destroyed_count;
  static inline int move_count;
  static inline int invoke_count;

  ~DestructableCallable() {
    if (!moved) {
      destroyed_count++;
    }
  }

  DestructableCallable() = default;

  DestructableCallable(DestructableCallable&& other) noexcept {
    other.moved = true;
    move_count++;
  }

  void operator()() const noexcept { invoke_count++; }

  bool moved = false;
};

struct ImplicitlyConvertibleResult {
  operator int() const { return value; }  // NOLINT(*explicit*)

  int value;
};

struct ExplicitlyConvertibleResult {
  explicit operator int() const { return value; }

  int value;
};

struct CallableReturningImplicitResult {
  ImplicitlyConvertibleResult operator()(int value) const { return {value}; }
};

struct CallableReturningExplicitResult {
  static inline int invoke_count;

  ExplicitlyConvertibleResult operator()(int value) const {
    invoke_count++;
    return {value};
  }
};

void test_DestructableCallable() {
  DestructableCallable::destroyed_count = 0;
  DestructableCallable::move_count = 0;
  DestructableCallable::invoke_count = 0;

  {
    using Func = SmallFunction<void()>;

    // 1 move while creation
    auto f1 = Func{DestructableCallable{}};
    // 1st invocation
    f1();
    // 2 move
    auto f2 = std::move(f1);
    // 2nd invocation
    f2();
    // actually destroyed only one DestructableCallable
  }
  TEST_ASSERT_EQUAL(1, DestructableCallable::destroyed_count);
  TEST_ASSERT_EQUAL(2, DestructableCallable::move_count);
  TEST_ASSERT_EQUAL(2, DestructableCallable::invoke_count);
}

void test_DestructableCallableInVector() {
  DestructableCallable::destroyed_count = 0;
  DestructableCallable::move_count = 0;
  DestructableCallable::invoke_count = 0;

  {
    using Func = SmallFunction<void()>;
    std::vector<Func> funcs;

    for (std::size_t i = 0; i < 255; ++i) {
      // each emplace should make vector grow and reallocate elements
      funcs.emplace_back(DestructableCallable{});
    }

    for (auto const& func : funcs) {
      func();
    }
  }
  TEST_ASSERT_EQUAL(255, DestructableCallable::destroyed_count);
  TEST_ASSERT_EQUAL(255, DestructableCallable::invoke_count);
}

class Worker {
 public:
  static inline int invoke_count;
  static inline int const_invoke_count;
  static inline int result_invoke_count;

  explicit Worker(int expected) : expected(expected) {}

  void Foo(int x) {
    invoke_count++;
    TEST_ASSERT_EQUAL(expected, x);
  }

  void FooConst(int x) const {
    const_invoke_count++;
    TEST_ASSERT_EQUAL(expected, x);
  }

  void FooRef(int const& x) {
    invoke_count++;
    TEST_ASSERT_EQUAL(expected, x);
  }

  void FooConstRef(int const& x) const {
    const_invoke_count++;
    TEST_ASSERT_EQUAL(expected, x);
  }

  // Intentionally non-static to exercise pointers to member functions.
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  int Return(int x) const { return x; }

  // Intentionally non-static to exercise MethodPtr member pointers.
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  ImplicitlyConvertibleResult ReturnImplicitResult(int x) const { return {x}; }

  // Intentionally non-static to exercise MethodPtr member pointers.
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  ExplicitlyConvertibleResult ReturnExplicitResult(int x) const {
    result_invoke_count++;
    return {x};
  }

  int expected;
};

void test_ClassMemberFunction() {
  using Func = SmallFunction<void(int x)>;
  using Wrapper = MethodPtr<&Worker::Foo>;
  constexpr int kTestValue = 12;
  static_assert(std::constructible_from<Func, Wrapper&>);
  static_assert(std::constructible_from<Func, Wrapper const&>);
  Worker::invoke_count = 0;
  Worker::const_invoke_count = 0;

  auto worker = Worker{kTestValue};
  auto wrapper = Wrapper{&worker};
  auto const const_wrapper = Wrapper{&worker};
  auto f1 = Func{wrapper};
  auto f2 = Func{MethodPtr<&Worker::FooConst>{&worker}};
  auto f3 = Func{const_wrapper};
  f1(kTestValue);
  f2(kTestValue);
  f3(kTestValue);
  TEST_ASSERT_EQUAL(2, Worker::invoke_count);
  TEST_ASSERT_EQUAL(1, Worker::const_invoke_count);

  auto f1_1 = std::move(f1);
  auto f2_1 = std::move(f2);
  f1_1(kTestValue);
  f2_1(kTestValue);
  TEST_ASSERT_EQUAL(3, Worker::invoke_count);
  TEST_ASSERT_EQUAL(2, Worker::const_invoke_count);
}

void test_ClassMemberFunctionRef() {
  using Func = SmallFunction<void(int const& x)>;
  static_assert(std::constructible_from<SmallFunction<void(int)>,
                                        MethodPtr<&Worker::FooRef>>);
  static_assert(std::constructible_from<SmallFunction<int(short)>,
                                        MethodPtr<&Worker::Return>>);
  static_assert(std::constructible_from<SmallFunction<void(int)>,
                                        MethodPtr<&Worker::Return>>);
  Worker::invoke_count = 0;
  Worker::const_invoke_count = 0;

  auto worker = Worker{12};
  auto f1 = Func{MethodPtr<&Worker::FooRef>{&worker}};
  auto f2 = Func{MethodPtr<&Worker::FooConstRef>{&worker}};
  f1(12);
  f2(12);
  TEST_ASSERT_EQUAL(1, Worker::invoke_count);
  TEST_ASSERT_EQUAL(1, Worker::const_invoke_count);

  auto f1_1 = std::move(f1);
  auto f2_1 = std::move(f2);
  f1_1(12);
  f2_1(12);
  TEST_ASSERT_EQUAL(2, Worker::invoke_count);
  TEST_ASSERT_EQUAL(2, Worker::const_invoke_count);
}

// Verifies generic callable and MethodPtr construction share implicit result
// conversion behavior while void wrappers discard invocation results.
void test_ReturnCompatibility() {
  using IntFunction = SmallFunction<int(int)>;
  using VoidFunction = SmallFunction<void(int)>;
  using ImplicitMethod = MethodPtr<&Worker::ReturnImplicitResult>;
  using ExplicitMethod = MethodPtr<&Worker::ReturnExplicitResult>;

  static_assert(
      std::constructible_from<IntFunction, CallableReturningImplicitResult>);
  static_assert(
      !std::constructible_from<IntFunction, CallableReturningExplicitResult>);
  static_assert(std::constructible_from<IntFunction, ImplicitMethod>);
  static_assert(!std::constructible_from<IntFunction, ExplicitMethod>);
  static_assert(
      std::constructible_from<VoidFunction, CallableReturningExplicitResult>);
  static_assert(std::constructible_from<VoidFunction, ExplicitMethod>);

  auto callable = IntFunction{CallableReturningImplicitResult{}};
  TEST_ASSERT_EQUAL(12, callable(12));

  auto worker = Worker{0};
  auto method = IntFunction{ImplicitMethod{&worker}};
  TEST_ASSERT_EQUAL(13, method(13));

  CallableReturningExplicitResult::invoke_count = 0;
  Worker::result_invoke_count = 0;
  constexpr int kCallableArgument = 14;
  constexpr int kMethodArgument = 15;
  auto void_callable = VoidFunction{CallableReturningExplicitResult{}};
  auto void_method = VoidFunction{ExplicitMethod{&worker}};
  void_callable(kCallableArgument);
  void_method(kMethodArgument);
  TEST_ASSERT_EQUAL(1, CallableReturningExplicitResult::invoke_count);
  TEST_ASSERT_EQUAL(1, Worker::result_invoke_count);
}

static inline int test_invoke_expected;
static inline int test_invoke_count;
static void TestInvoke(int x) {
  test_invoke_count++;
  TEST_ASSERT_EQUAL(test_invoke_expected, x);
}

void test_FreeFunction() {
  using Func = SmallFunction<void(int x)>;
  test_invoke_expected = 12;
  test_invoke_count = 0;

  printf("TestInvoke addr %p\n", TestInvoke);

  auto f1 = Func{TestInvoke};
  f1(12);
  TEST_ASSERT_EQUAL(1, test_invoke_count);

  auto f1_1 = std::move(f1);
  f1_1(12);
  TEST_ASSERT_EQUAL(2, test_invoke_count);

  auto fl1 = Func{+[](int x) {
    test_invoke_count++;
    TEST_ASSERT_EQUAL(test_invoke_expected, x);
  }};

  fl1(12);
  TEST_ASSERT_EQUAL(3, test_invoke_count);
}

}  // namespace ae::test_small_function

int test_small_function() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_small_function::test_CallALambdas);
  RUN_TEST(ae::test_small_function::test_ConstWrapperCallsMutableCallable);
  RUN_TEST(ae::test_small_function::test_FunctionsInVector);
  RUN_TEST(ae::test_small_function::test_DestructableCallable);
  RUN_TEST(ae::test_small_function::test_DestructableCallableInVector);
  RUN_TEST(ae::test_small_function::test_ClassMemberFunction);
  RUN_TEST(ae::test_small_function::test_ClassMemberFunctionRef);
  RUN_TEST(ae::test_small_function::test_ReturnCompatibility);
  RUN_TEST(ae::test_small_function::test_FreeFunction);
  return UNITY_END();
}

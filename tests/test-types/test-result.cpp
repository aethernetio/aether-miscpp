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

#include "aether-miscpp/types/result.h"

namespace ae::test_result {
struct G {};
struct E {};

void test_Result() {
  auto res = Result<G, E>{G{}};
  // check if concept works
  static_assert(ae::ResultType<decltype(res), G, E>);
  TEST_ASSERT_TRUE(res.IsOk());

  auto res_e = Result<G, E>{E{}};
  TEST_ASSERT_TRUE(res_e.IsErr());
}

void test_ResultOkReference() {
  // Result<T&, E> stores Ok as a reference and exposes the original object.
  int value = 7;

  auto res = Result<int&, E>{Ok<int&>{value}};

  static_assert(ae::ResultType<decltype(res), int&, E>);
  static_assert(std::is_same_v<decltype(res.value()), int&>);

  TEST_ASSERT_TRUE(res.IsOk());
  TEST_ASSERT_EQUAL_PTR(&value, &res.value());

  res.value() = 11;
  TEST_ASSERT_EQUAL_INT(11, value);

  value = 13;
  TEST_ASSERT_EQUAL_INT(13, res.value());
}

void test_ResultErrorReference() {
  // Result<T, E&> stores Error as a reference and exposes the original object.
  int error = 17;

  auto res = Result<G, int&>{Error<int&>{error}};

  static_assert(ae::ResultType<decltype(res), G, int&>);
  static_assert(std::is_same_v<decltype(res.error()), int&>);

  TEST_ASSERT_TRUE(res.IsErr());
  TEST_ASSERT_EQUAL_PTR(&error, &res.error());

  res.error() = 23;
  TEST_ASSERT_EQUAL_INT(23, error);

  error = 29;
  TEST_ASSERT_EQUAL_INT(29, res.error());
}

void test_ResultSameValueAndErrorTypes() {
  // Same Ok/Error payload types are disambiguated by Ok<T> and Error<E> tags.
  auto ok = Result<int, int>{Ok<int>{7}};
  TEST_ASSERT_TRUE(ok.IsOk());
  TEST_ASSERT_EQUAL_INT(7, ok.value());

  auto err = Result<int, int>{Error<int>{13}};
  TEST_ASSERT_TRUE(err.IsErr());
  TEST_ASSERT_EQUAL_INT(13, err.error());
}

void test_Monadic() {
  auto ret_good = []() -> Result<G, E> { return Ok{G{}}; };
  auto ret_bad = []() -> Result<G, E> { return Error{E{}}; };

  auto res_g = ret_good().Then(ret_good);
  TEST_ASSERT_TRUE(res_g.IsOk());

  auto res_e = ret_good().Then(ret_bad);
  TEST_ASSERT_TRUE(res_e.IsErr());

  auto res_g2 = ret_bad().Else(ret_good);
  TEST_ASSERT_TRUE(res_g2.IsOk());
  auto res_e2 = ret_bad().Else(ret_bad);
  TEST_ASSERT_TRUE(res_e2.IsErr());

  auto res_cg = ret_good().Then(ret_good).Else(ret_bad);
  TEST_ASSERT_TRUE(res_cg.IsOk());
  auto res_cg2 = ret_good().Then(ret_bad).Else(ret_good);
  TEST_ASSERT_TRUE(res_cg2.IsOk());

  auto res_ce = ret_bad().Then(ret_good).Else(ret_bad);
  TEST_ASSERT_TRUE(res_ce.IsErr());
  auto res_ce2 = ret_bad().Else(ret_good).Else(ret_bad);
  TEST_ASSERT_TRUE(res_ce2.IsOk());
}

void test_Macros() {
  auto try_value = [](int v, int n) -> Result<G, E> {
    if (v > n) {
      return Ok{G{}};
    }
    return Error{E{}};
  };

  auto test_good = [&]() -> Result<G, E> {
    TRY_VALUE(rg, try_value(12, 10));
    return Ok{rg};
  }();
  TEST_ASSERT_TRUE(test_good.IsOk());

  auto test_bad = [&]() -> Result<G, E> {
    TRY_VALUE(rg, try_value(10, 12));
    return Ok{rg};
  }();
  TEST_ASSERT_TRUE(test_bad.IsErr());

  auto test_good_res = [&]() -> Result<G, E> {
    TRY_RESULT(try_value(42, 5));
    return Ok{G{}};
  }();
  TEST_ASSERT_TRUE(test_good_res.IsOk());

  auto test_bad_res = [&]() -> Result<G, E> {
    TRY_RESULT(try_value(1, 105));
    return Ok{G{}};
  }();
  TEST_ASSERT_TRUE(test_bad_res.IsErr());
}

}  // namespace ae::test_result

int test_result() {
  UNITY_BEGIN();
  using namespace ae::test_result;  // NOLINT

  RUN_TEST(test_Result);
  RUN_TEST(test_ResultOkReference);
  RUN_TEST(test_ResultErrorReference);
  RUN_TEST(test_ResultSameValueAndErrorTypes);
  RUN_TEST(test_Monadic);
  RUN_TEST(test_Macros);

  return UNITY_END();
}

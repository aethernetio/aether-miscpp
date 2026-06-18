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
  RUN_TEST(test_Monadic);
  RUN_TEST(test_Macros);

  return UNITY_END();
}

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
#include <tuple>
#include <type_traits>
#include <utility>

#include <unity.h>

#include "aether-miscpp/reflect/reflect.h"

namespace {
struct Sample {
  int value{3};
  int ref_value{9};

  AE_REFLECT(AE_MMBR(value), AE_REF(ref_value))
};
}  // namespace

int test_ReflectReflection() {
  UNITY_BEGIN();

  auto sample = Sample{};
  auto reflection = ae::reflect::make_reflection(sample);
  static_assert(ae::reflect::Reflectable<Sample>);
  TEST_ASSERT_EQUAL(2, reflection.size());
  TEST_ASSERT_EQUAL(3, reflection.get<0>());
  TEST_ASSERT_EQUAL(9, reflection.get<1>());

  auto sum =
      reflection.Apply([](auto const& a, auto const& b) { return a + b; });
  TEST_ASSERT_EQUAL(12, sum);

  auto name0 = reflection.name<0>();
  auto name1 = reflection.name<1>();
  TEST_ASSERT_EQUAL_STRING("value", name0.data());
  TEST_ASSERT_EQUAL_STRING("ref_value", name1.data());

  auto names = reflection.ApplyName(
      [](auto const&... names) { return std::tuple{names...}; });
  TEST_ASSERT_EQUAL_STRING("value", std::get<0>(names).data());
  TEST_ASSERT_EQUAL_STRING("ref_value", std::get<1>(names).data());

  auto meta_sum = reflection.ApplyMeta([](auto&& obj, auto const&... metas) {
    return (metas.get(std::forward<decltype(obj)>(obj)) + ...);
  });
  TEST_ASSERT_EQUAL(12, meta_sum);

  auto const const_sample = Sample{};
  auto const_object_reflection = ae::reflect::make_reflection(const_sample);
  static_assert(
      std::is_same_v<decltype(const_object_reflection.get<0>()), int const&>);
  static_assert(
      std::is_same_v<decltype(const_object_reflection.get<1>()), int const&>);
  TEST_ASSERT_EQUAL(3, const_object_reflection.get<0>());
  TEST_ASSERT_EQUAL(9, const_object_reflection.get<1>());

  auto rvalue_reflection = ae::reflect::make_reflection(Sample{});
  TEST_ASSERT_EQUAL(3, rvalue_reflection.get<0>());
  static_assert(std::is_same_v<decltype(rvalue_reflection.get<0>()), int&&>);
  static_assert(std::is_same_v<decltype(rvalue_reflection.get<1>()), int&>);

  return UNITY_END();
}

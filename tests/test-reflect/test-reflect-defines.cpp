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

#include <unity.h>

#include "aether-miscpp/reflect/reflect.h"

namespace {
struct EmptyMacro {
  AE_REFLECT()
};

struct Empty {
  template <typename SelfType>
  static constexpr auto Mirror() noexcept {
    return ae::reflect::Mirror{ae::reflect::ClassTag<SelfType>{}};
  }
};

struct Base {
  int base_value{4};

  AE_REFLECT_MEMBERS(base_value)
};

struct Sample : Base {
  int value{8};
  int& ref;

  explicit Sample(int& r) : ref(r) {}

  AE_REFLECT(AE_BASE(Base), AE_MMBR(value), AE_REF(ref))
};

struct BaseOnlyBase {
  int inherited{21};

  AE_REFLECT_MEMBERS(inherited)
};

struct BaseOnlyDerived : BaseOnlyBase {
  AE_REFLECT(AE_BASE(BaseOnlyBase))
};
}  // namespace

int test_ReflectDefines() {
  UNITY_BEGIN();

  int ref_value = 15;
  auto sample = Sample{ref_value};
  auto reflection = ae::reflect::make_reflection(sample);
  TEST_ASSERT_EQUAL(3, reflection.size());
  TEST_ASSERT_EQUAL(4, reflection.get<0>());
  TEST_ASSERT_EQUAL(8, reflection.get<1>());
  TEST_ASSERT_EQUAL(15, reflection.get<2>());

  TEST_ASSERT_EQUAL_STRING("base_value", reflection.name<0>().data());
  TEST_ASSERT_EQUAL_STRING("value", reflection.name<1>().data());
  TEST_ASSERT_EQUAL_STRING("ref", reflection.name<2>().data());

  auto values =
      reflection.Apply([](auto&&... values) { return std::tuple{values...}; });
  TEST_ASSERT_EQUAL(4, std::get<0>(values));
  TEST_ASSERT_EQUAL(8, std::get<1>(values));
  TEST_ASSERT_EQUAL(15, std::get<2>(values));

  static_assert(ae::reflect::Reflectable<Empty>);
  auto empty = Empty{};
  auto empty_reflection = ae::reflect::make_reflection(empty);
  TEST_ASSERT_EQUAL(0, empty_reflection.size());
  auto empty_count = empty_reflection.Apply([]() { return 0; });
  TEST_ASSERT_EQUAL(0, empty_count);

  static_assert(ae::reflect::Reflectable<EmptyMacro>);
  auto empty_macro = EmptyMacro{};
  auto empty_macro_reflection = ae::reflect::make_reflection(empty_macro);
  TEST_ASSERT_EQUAL(0, empty_macro_reflection.size());
  auto empty_macro_apply = empty_macro_reflection.Apply([]() { return 0; });
  TEST_ASSERT_EQUAL(0, empty_macro_apply);
  auto empty_macro_name = empty_macro_reflection.ApplyName([]() { return 0; });
  TEST_ASSERT_EQUAL(0, empty_macro_name);
  auto empty_macro_meta = empty_macro_reflection.ApplyMeta(
      [](auto&&... args) { return static_cast<int>(sizeof...(args)); });
  TEST_ASSERT_EQUAL(0, empty_macro_meta);

  BaseOnlyDerived derived{};
  auto derived_reflection = ae::reflect::make_reflection(derived);
  TEST_ASSERT_EQUAL(1, derived_reflection.size());
  TEST_ASSERT_EQUAL_STRING("inherited", derived_reflection.name<0>().data());
  TEST_ASSERT_EQUAL(21, derived_reflection.get<0>());
  derived_reflection.get<0>() = 99;
  TEST_ASSERT_EQUAL(99, derived.inherited);

  return UNITY_END();
}

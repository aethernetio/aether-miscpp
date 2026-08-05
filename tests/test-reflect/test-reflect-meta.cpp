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
#include <string_view>

#include <unity.h>

#include "aether-miscpp/reflect/reflect.h"

namespace {
struct Sample {
  int value{};
  int getter_value{0};

  int* get_value() { return &getter_value; }
  int const* get_value() const { return &getter_value; }
};
}  // namespace

int test_ReflectMeta() {
  UNITY_BEGIN();

  using Member = ae::reflect::MetaMember<Sample, int>;
  static_assert(ae::reflect::MetaType<Member>);

  Sample sample{7, 11};
  auto member = Member{"value", &Sample::value};
  TEST_ASSERT_EQUAL(7, member.get(sample));
  TEST_ASSERT_EQUAL_STRING("value", member.name.data());

  using Getter = ae::reflect::MetaGetter<Sample, int* (Sample::*)()>;
  static_assert(ae::reflect::MetaType<Getter>);

  auto getter = Getter{"getter_value",
                       static_cast<int* (Sample::*)()>(&Sample::get_value)};
  TEST_ASSERT_EQUAL(11, getter.get(sample));
  TEST_ASSERT_EQUAL_STRING("getter_value", getter.name.data());

  using ConstGetter =
      ae::reflect::MetaGetter<Sample, int const* (Sample::*)() const>;
  static_assert(ae::reflect::MetaType<ConstGetter>);

  const Sample const_sample{7, 11};
  auto const_getter = ConstGetter{
      "getter_value",
      static_cast<int const* (Sample::*)() const>(&Sample::get_value)};
  TEST_ASSERT_EQUAL(11, const_getter.get(const_sample));

  return UNITY_END();
}

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
struct Empty {
  template <typename SelfType>
  static constexpr auto Mirror() noexcept {
    return ae::reflect::Mirror{ae::reflect::ClassTag<SelfType>{}};
  }
};

struct Sample {
  int value{};
  int other{};

  template <typename SelfType>
  static constexpr auto Mirror() noexcept {
    return ae::reflect::Mirror{
        ae::reflect::ClassTag<SelfType>{},
        ae::reflect::MetaMember<SelfType, int>{"value", &SelfType::value},
        ae::reflect::MetaMember<SelfType, int>{"other", &SelfType::other}};
  }
};
}  // namespace

int test_ReflectMirror() {
  UNITY_BEGIN();

  using Mirror = decltype(Sample::Mirror<Sample>());
  static_assert(ae::reflect::MirrorType<Mirror>);
  using EmptyMirror = decltype(Empty::Mirror<Empty>());
  static_assert(ae::reflect::MirrorType<EmptyMirror>);

  auto empty_mirror = Empty::Mirror<Empty>();
  TEST_ASSERT_EQUAL(0, empty_mirror.size());
  TEST_ASSERT_EQUAL(0, decltype(empty_mirror)::kSize);
  auto count = empty_mirror.Apply([]() { return 0; });
  TEST_ASSERT_EQUAL(0, count);

  auto mirror = Sample::Mirror<Sample>();
  TEST_ASSERT_EQUAL(2, mirror.size());
  TEST_ASSERT_EQUAL(2, decltype(mirror)::kSize);
  TEST_ASSERT_EQUAL_STRING("value", mirror.get<0>().name.data());
  TEST_ASSERT_EQUAL_STRING("other", mirror.get<1>().name.data());

  auto names = mirror.Apply(
      [](auto const&... metas) { return std::tuple{metas.name...}; });
  TEST_ASSERT_EQUAL_STRING("value", std::get<0>(names).data());
  TEST_ASSERT_EQUAL_STRING("other", std::get<1>(names).data());

  return UNITY_END();
}

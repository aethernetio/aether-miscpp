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

#include "aether-miscpp/serialization/details/meta.h"

using ae::seri::Meta;

namespace test_meta {
template <typename T>
concept HasMetaValueType = requires { typename ae::seri::MetaValueType_t<T>; };

// Validates Meta deduction, reference preservation, and IsMetaType direct
// recognition.
void test_Meta() {
  int v = 1;
  const int cv = 2;
  auto m1 = Meta{v};
  auto m2 = Meta{cv};
  auto m3 = Meta{v, "n"};
  static_assert(std::is_same_v<decltype(m1.value), int&>);
  static_assert(std::is_same_v<decltype(m2.value), int const&>);
  static_assert(ae::seri::IsMetaType_v<decltype(m1)>);
  static_assert(!ae::seri::IsMetaType_v<decltype(m1)&>);
  static_assert(!ae::seri::IsMetaType_v<decltype(m2) const&>);
  static_assert(ae::seri::IsMetaType_v<decltype(ae::seri::Meta{v})>);
  static_assert(ae::seri::IsMetaType_v<decltype(ae::seri::Meta{cv})>);
  static_assert(std::is_same_v<ae::seri::MetaValueType_t<Meta<int>>, int>);
  static_assert(
      std::is_same_v<ae::seri::MetaValueType_t<Meta<int const>>, int const>);
  static_assert(
      std::is_same_v<ae::seri::MetaValueType_t<Meta<int> const>, int>);
  static_assert(!HasMetaValueType<Meta<int>&>);
  static_assert(!HasMetaValueType<Meta<int>&&>);
  (void)m3;
}
}  // namespace test_meta

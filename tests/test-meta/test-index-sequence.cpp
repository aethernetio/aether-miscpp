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

#include <utility>

#include "aether-miscpp/meta/index_sequence.h"

namespace ae::test_index_sequence {

void test_ReverseSequence() {
  constexpr auto rev_indices =
      ae::reverse_sequence(std::make_index_sequence<10>());
  static_assert(
      std::is_same_v<std::index_sequence<9, 8, 7, 6, 5, 4, 3, 2, 1, 0>,
                     std::decay_t<decltype(rev_indices)>>);

  constexpr auto types_rev_indices =
      ae::reverse_sequence(std::index_sequence_for<int, float, bool, double>());
  static_assert(std::is_same_v<std::index_sequence<3, 2, 1, 0>,
                               std::decay_t<decltype(types_rev_indices)>>);
  TEST_PASS();
}

}  // namespace ae::test_index_sequence

int test_index_sequence() {
  UNITY_BEGIN();
  RUN_TEST(ae::test_index_sequence::test_ReverseSequence);
  return UNITY_END();
}

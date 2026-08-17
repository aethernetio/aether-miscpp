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
#include <cstddef>
#include <utility>

#include "aether-miscpp/serialization/details/tags.h"

using ae::seri::DataTag;
using ae::seri::SizeTag;

namespace test_tags {
// Validates SizeTag/DataTag construction for mutable, const, and raw buffers.
void test_Tags() {
  constexpr int kMutableValue = 7;
  std::size_t s = 3;
  auto st = SizeTag{s};
  auto dw = DataTag{std::as_const(s)};
  int v = kMutableValue;
  auto dr = DataTag{v};
  auto dv = DataTag{static_cast<void*>(nullptr), std::size_t{0}};
  (void)st;
  (void)dw;
  (void)dr;
  (void)dv;
}
}  // namespace test_tags

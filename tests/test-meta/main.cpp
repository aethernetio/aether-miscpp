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

void setUp() {}
void tearDown() {}

extern int test_type_list();
extern int test_arg_at();
extern int test_index_sequence();
extern int test_as_type();
extern int test_TypeIndex();

int main() {
  int res{};
  res += test_type_list();
  res += test_arg_at();
  res += test_index_sequence();
  res += test_as_type();
  res += test_TypeIndex();
  return res;
}

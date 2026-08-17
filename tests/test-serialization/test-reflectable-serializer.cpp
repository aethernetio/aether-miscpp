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

#include "aether-miscpp/serialization/details/binary_archive.h"
#include "aether-miscpp/serialization/details/reflectable_serializer.h"


namespace test_reflectable_serializer {
struct EmptyReflectable {
  AE_REFLECT();
};

struct CountingBuffer {
  int writes{};
  int reads{};
  ae::Result<ae::seri::Good, ae::seri::SeriError> Write(
      ae::seri::SizeWriteTag) {
    return ae::Ok{ae::seri::good};
  }
  ae::Result<ae::seri::Good, ae::seri::SeriError> Write(
      ae::seri::DataWriteTag) {
    ++writes;
    return ae::Ok{ae::seri::good};
  }
  ae::Result<ae::seri::Good, ae::seri::SeriError> Read(ae::seri::SizeReadTag) {
    return ae::Ok{ae::seri::good};
  }
  ae::Result<ae::seri::Good, ae::seri::SeriError> Read(ae::seri::DataReadTag) {
    ++reads;
    return ae::Ok{ae::seri::good};
  }
};
// Validates empty reflected types serialize/deserialize without data reads or
// writes.
void test_ReflectableSerializer() {
  ae::seri::BinaryArchive<CountingBuffer> archive{CountingBuffer{}};
  EmptyReflectable value{};
  TEST_ASSERT_TRUE(archive.Save(value));
  TEST_ASSERT_EQUAL(0, archive.buffer().writes);
  TEST_ASSERT_TRUE(archive.Load(value));
  TEST_ASSERT_EQUAL(0, archive.buffer().reads);
}
}  // namespace test_reflectable_serializer

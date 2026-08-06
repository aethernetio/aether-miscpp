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

namespace test_meta {
void test_Meta();
}
namespace test_tags {
void test_Tags();
}
namespace test_archive_facade {
void test_ArchiveFacade();
}
namespace test_binary_archive {
void test_BinaryArchive();
}
namespace test_binary_std_serializers {
void test_BinaryStdSerializers();
}
namespace test_binary_vector_buffer {
void test_BinaryVectorBuffer();
}
namespace test_reflectable_serializer {
void test_ReflectableSerializer();
}
namespace test_member_serializer {
void test_MemberSerializer();
}
namespace test_public_headers {
void test_PublicHeaders();
}

void setUp(void) {}
void tearDown(void) {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_meta::test_Meta);
  RUN_TEST(test_tags::test_Tags);
  RUN_TEST(test_archive_facade::test_ArchiveFacade);
  RUN_TEST(test_binary_archive::test_BinaryArchive);
  RUN_TEST(test_binary_std_serializers::test_BinaryStdSerializers);
  RUN_TEST(test_binary_vector_buffer::test_BinaryVectorBuffer);
  RUN_TEST(test_reflectable_serializer::test_ReflectableSerializer);
  RUN_TEST(test_member_serializer::test_MemberSerializer);
  RUN_TEST(test_public_headers::test_PublicHeaders);
  return UNITY_END();
}

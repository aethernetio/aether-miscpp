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
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include <unity.h>

#include "aether-miscpp/reflect/reflect.h"
#include "aether-miscpp/serialization/binary_archive.h"
#include "aether-miscpp/serialization/serialization.h"

namespace test_public_headers {
using ae::Ok;
using ae::Result;
using ae::seri::BinaryArchive;
using ae::seri::BinaryBuffer;
using ae::seri::DataReadTag;
using ae::seri::DataWriteTag;
using ae::seri::Good;
using ae::seri::SeriError;
using ae::seri::SizeReadTag;
using ae::seri::SizeWriteTag;

struct Buffer {
  std::vector<std::byte> bytes;
  std::size_t read_pos{};

  Result<Good, SeriError> Write(SizeWriteTag tag) {
    auto const* ptr = reinterpret_cast<std::byte const*>(&tag.size);
    bytes.insert(bytes.end(), ptr, ptr + sizeof(tag.size));
    return ae::Ok{ae::seri::good};
  }
  Result<Good, SeriError> Write(DataWriteTag tag) {
    auto const* ptr = reinterpret_cast<std::byte const*>(tag.data);
    bytes.insert(bytes.end(), ptr, ptr + tag.size);
    return ae::Ok{ae::seri::good};
  }
  Result<Good, SeriError> Read(SizeReadTag tag) {
    std::memcpy(&tag.size, bytes.data() + read_pos, sizeof(tag.size));
    read_pos += sizeof(tag.size);
    return ae::Ok{ae::seri::good};
  }
  Result<Good, SeriError> Read(DataReadTag tag) {
    std::memcpy(tag.data, bytes.data() + read_pos, tag.size);
    read_pos += tag.size;
    return ae::Ok{ae::seri::good};
  }
};

static_assert(BinaryBuffer<Buffer>);
static_assert(ae::seri::Archive<BinaryArchive<Buffer>>);

enum class Kind : std::uint8_t { A = 1, B = 3 };

struct Record {
  int value{};
  double weight{};
  Kind kind{};
  std::string name{};
  std::vector<std::uint32_t> ids{};
  std::optional<int> tag{};
  AE_REFLECT_MEMBERS(value, weight, kind, name, ids, tag)
};
// Validates public serialization headers support reflected records with scalar,
// enum, string, vector, and optional members.
void test_PublicHeaders() {
  BinaryArchive<Buffer> archive{Buffer{}};

  Record saved{42, 3.5, Kind::B, "hello", {7u, 9u, 11u}, 5};
  TEST_ASSERT_TRUE(archive.Save(saved));

  Record loaded{};
  TEST_ASSERT_TRUE(archive.Load(loaded));
  TEST_ASSERT_EQUAL(saved.value, loaded.value);
  TEST_ASSERT_EQUAL_DOUBLE(saved.weight, loaded.weight);
  TEST_ASSERT_EQUAL(static_cast<int>(saved.kind),
                    static_cast<int>(loaded.kind));
  TEST_ASSERT_EQUAL_STRING(saved.name.c_str(), loaded.name.c_str());
  TEST_ASSERT_TRUE(saved.ids == loaded.ids);
  TEST_ASSERT_TRUE(loaded.tag.has_value());
}
}  // namespace test_public_headers

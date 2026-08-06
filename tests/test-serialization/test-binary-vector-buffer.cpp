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
#include <vector>

#include <unity.h>

#include "aether-miscpp/serialization/binary_archive.h"

namespace test_binary_vector_buffer {
static_assert(ae::seri::BinaryBuffer<ae::seri::BinaryVectorBuffer<>>);
static_assert(ae::seri::BinaryBuffer<ae::seri::LimitedVectorBuffer<>>);

struct Pod {
  std::uint32_t value{};
};

struct CustomSizeType {
  CustomSizeType() = default;

  explicit CustomSizeType(std::size_t value) : value(value) {}

  explicit operator std::size_t() const { return value; }

  std::size_t value{};
};

// Covers vector-backed binary buffers, including size encoding, read cursor
// behavior, and limited-capacity EOF handling.
void test_BinaryVectorBuffer() {
  {
    std::vector<std::uint8_t> bytes;
    ae::seri::BinaryVectorBuffer<> buffer{bytes};
    ae::seri::BinaryArchive archive{buffer};
    Pod value{0x01020304u};
    TEST_ASSERT_TRUE(archive.Save(value.value));
    TEST_ASSERT_EQUAL(sizeof(std::uint32_t), bytes.size());
    TEST_ASSERT_EQUAL_UINT8(0x04, bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0x03, bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(0x02, bytes[2]);
    TEST_ASSERT_EQUAL_UINT8(0x01, bytes[3]);
    archive.buffer().set_read_position(0);
    std::uint32_t loaded{};
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL(value.value, loaded);
  }

  {
    std::vector<std::uint8_t> bytes;
    ae::seri::BinaryVectorBuffer<std::uint8_t> buffer{bytes};
    TEST_ASSERT_TRUE(buffer.Write(ae::seri::SizeWriteTag{256}));
    TEST_ASSERT_EQUAL(1, bytes.size());
    TEST_ASSERT_EQUAL_UINT8(0x00, bytes[0]);
  }

  {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(1);
    ae::seri::LimitedVectorBuffer<std::uint8_t> buffer{bytes};
    TEST_ASSERT_FALSE(buffer.eof());
    TEST_ASSERT_TRUE(buffer.Write(ae::seri::SizeWriteTag{256}));
    TEST_ASSERT_TRUE(buffer.eof());
    TEST_ASSERT_EQUAL(1, bytes.size());
    TEST_ASSERT_EQUAL_UINT8(0x00, bytes[0]);
  }

  {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(1);
    ae::seri::LimitedVectorBuffer<std::uint16_t> buffer{bytes};
    auto const before_size = bytes.size();
    auto result = buffer.Write(ae::seri::SizeWriteTag{1});
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::write_eof.error_code,
                      result.error().error_code);
    TEST_ASSERT_TRUE(buffer.eof());
    TEST_ASSERT_EQUAL(before_size, bytes.size());
  }

  {
    std::vector<std::uint8_t> bytes{0x01};
    ae::seri::BinaryVectorBuffer<> buffer{bytes};
    std::size_t size{};
    auto result = buffer.Read(ae::seri::SizeReadTag{size});
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::read_eof.error_code, result.error().error_code);
    TEST_ASSERT_EQUAL(0u, buffer.read_position());
    std::uint8_t data{};
    result = buffer.Read(ae::seri::DataReadTag{&data, 2});
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::read_eof.error_code, result.error().error_code);
    TEST_ASSERT_EQUAL(0u, buffer.read_position());
  }

  {
    std::vector<std::uint8_t> bytes;
    ae::seri::BinaryVectorBuffer<bool> buffer{bytes};
    TEST_ASSERT_TRUE(buffer.Write(ae::seri::SizeWriteTag{1}));
    TEST_ASSERT_EQUAL(sizeof(bool), bytes.size());
  }

  {
    std::vector<std::uint8_t> bytes;
    ae::seri::BinaryVectorBuffer<CustomSizeType> buffer{bytes};
    TEST_ASSERT_TRUE(buffer.Write(ae::seri::SizeWriteTag{42}));
    std::size_t loaded_size{};
    buffer.set_read_position(0);
    auto result = buffer.Read(ae::seri::SizeReadTag{loaded_size});
    TEST_ASSERT_TRUE(result);
    CustomSizeType restored{loaded_size};
    TEST_ASSERT_EQUAL(42u, restored.value);
  }

  {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(1);
    ae::seri::LimitedVectorBuffer<std::uint8_t> buffer{bytes};
    TEST_ASSERT_FALSE(buffer.eof());
    auto const capacity = bytes.capacity();
    std::vector<std::uint8_t> data(capacity, 0x61);
    TEST_ASSERT_TRUE(
        buffer.Write(ae::seri::DataWriteTag{data.data(), capacity}));
    TEST_ASSERT_TRUE(buffer.eof());
    std::uint8_t b{0x62};
    auto result = buffer.Write(ae::seri::DataWriteTag{&b, 1});
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::write_eof.error_code,
                      result.error().error_code);
  }

  {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(1);
    ae::seri::LimitedVectorBuffer<std::uint8_t> buffer{bytes};
    auto const capacity = bytes.capacity();
    std::vector<std::uint8_t> data(capacity + 1, 0x61);
    auto result =
        buffer.Write(ae::seri::DataWriteTag{data.data(), data.size()});
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::write_eof.error_code,
                      result.error().error_code);
    TEST_ASSERT_TRUE(buffer.eof());
    TEST_ASSERT_EQUAL_UINT(0, bytes.size());
  }
}
}  // namespace test_binary_vector_buffer

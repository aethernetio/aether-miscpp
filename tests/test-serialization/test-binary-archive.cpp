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

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <unity.h>

#include "aether-miscpp/reflect/reflect.h"
#include "aether-miscpp/serialization/binary_archive.h"
#include "aether-miscpp/serialization/details/meta.h"
#include "aether-miscpp/serialization/details/reflectable_serializer.h"
#include "aether-miscpp/serialization/details/serialization_result.h"

namespace test_binary_archive {
using ae::Error;
using ae::Ok;
using ae::Result;
using ae::seri::DataReadTag;
using ae::seri::DataWriteTag;
using ae::seri::Good;
using ae::seri::SeriError;
using ae::seri::SizeReadTag;
using ae::seri::SizeWriteTag;

struct MemoryBuffer {
  std::vector<std::byte> bytes;
  std::size_t read_pos{};
  int read_calls{};
  int write_calls{};
  int size_read_calls{};
  int data_read_calls{};
  int size_write_calls{};
  int data_write_calls{};
  int fail_on_read_call{};
  int fail_on_write_call{};
  std::size_t write_limit{std::numeric_limits<std::size_t>::max()};

  Result<Good, SeriError> Write(SizeWriteTag tag) {
    ++write_calls;
    ++size_write_calls;
    if (fail_on_write_call == write_calls) {
      return Error{ae::seri::write_error};
    }
    if (bytes.size() + sizeof(tag.size) > write_limit) {
      return Error{ae::seri::write_eof};
    }
    auto const* ptr = reinterpret_cast<std::byte const*>(&tag.size);
    bytes.insert(bytes.end(), ptr, ptr + sizeof(tag.size));
    return ae::Ok{ae::seri::good};
  }
  Result<Good, SeriError> Write(DataWriteTag tag) {
    ++write_calls;
    ++data_write_calls;
    if (fail_on_write_call == write_calls) {
      return Error{ae::seri::write_error};
    }
    if (bytes.size() + tag.size > write_limit) {
      return Error{ae::seri::write_eof};
    }
    auto const* ptr = reinterpret_cast<std::byte const*>(tag.data);
    bytes.insert(bytes.end(), ptr, ptr + tag.size);
    return ae::Ok{ae::seri::good};
  }
  Result<Good, SeriError> Read(SizeReadTag tag) {
    ++read_calls;
    ++size_read_calls;
    if (fail_on_read_call == read_calls) {
      return Error{ae::seri::read_error};
    }
    if (read_pos + sizeof(tag.size) > bytes.size()) {
      return Error{ae::seri::read_eof};
    }
    std::memcpy(&tag.size, bytes.data() + read_pos, sizeof(tag.size));
    read_pos += sizeof(tag.size);
    return ae::Ok{ae::seri::good};
  }
  Result<Good, SeriError> Read(DataReadTag tag) {
    ++read_calls;
    ++data_read_calls;
    if (fail_on_read_call == read_calls) {
      return Error{ae::seri::read_error};
    }
    if (read_pos + tag.size > bytes.size()) {
      return Error{ae::seri::read_eof};
    }
    std::memcpy(tag.data, bytes.data() + read_pos, tag.size);
    read_pos += tag.size;
    return ae::Ok{ae::seri::good};
  }
};

static_assert(ae::seri::BinaryBuffer<MemoryBuffer>);

enum class TestEnum : std::uint16_t { A = 7, B = 19 };

struct Simple {
  int32_t i{};
  double d{};
  TestEnum e{};
  AE_REFLECT_MEMBERS(i, d, e)
};

struct Base {
  int32_t b{};
  AE_REFLECT_MEMBERS(b)
};

struct Derived : Base {
  double x{};
  TestEnum e{};
  AE_REFLECT(AE_BASE(Base), AE_MMBR(x), AE_MMBR(e))
};

template <typename T>
std::vector<std::byte> raw_bytes(T const& value) {
  auto const* ptr = reinterpret_cast<std::byte const*>(&value);
  return {ptr, ptr + sizeof(T)};
}

// Validates binary archive round trips, error propagation, bool encoding, and
// container limits.
void test_BinaryArchive() {
  using Archive = ae::seri::BinaryArchive<MemoryBuffer>;

  {
    // Exact call counts are only asserted for fail-on-call propagation,
    // oversize short-circuiting, explicit bool encoding, and vector<bool>
    // fallback.
    Archive archive{MemoryBuffer{}};
    int32_t i = -123;
    double d = 4.25;
    TestEnum e = TestEnum::B;
    TEST_ASSERT_TRUE(archive.Save(i));
    TEST_ASSERT_TRUE(archive.Save(d));
    TEST_ASSERT_TRUE(archive.Save(e));
    TEST_ASSERT_EQUAL_UINT(sizeof(i) + sizeof(d) + sizeof(std::uint16_t),
                           archive.buffer().bytes.size());
    auto expected = raw_bytes(i);
    auto expected_d = raw_bytes(d);
    auto underlying = static_cast<std::uint16_t>(e);
    auto expected_e = raw_bytes(underlying);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        expected.data(), archive.buffer().bytes.data(), expected.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        expected_d.data(), archive.buffer().bytes.data() + expected.size(),
        expected_d.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        expected_e.data(),
        archive.buffer().bytes.data() + expected.size() + expected_d.size(),
        expected_e.size());

    int32_t oi{};
    double od{};
    TestEnum oe{};
    TEST_ASSERT_TRUE(archive.Load(oi));
    TEST_ASSERT_TRUE(archive.Load(od));
    TEST_ASSERT_TRUE(archive.Load(oe));
    TEST_ASSERT_EQUAL(i, oi);
    TEST_ASSERT_EQUAL_DOUBLE(d, od);
    TEST_ASSERT_EQUAL((int)e, (int)oe);
  }

  {
    Archive archive{MemoryBuffer{}};
    Simple s{42, 1.5, TestEnum::A};
    Derived der{{9}, 2.5, TestEnum::B};
    TEST_ASSERT_TRUE(archive.Save(s));
    TEST_ASSERT_TRUE(archive.Save(der));
    Simple os{};
    Derived oder{};
    TEST_ASSERT_TRUE(archive.Load(os));
    TEST_ASSERT_TRUE(archive.Load(oder));
    TEST_ASSERT_EQUAL(s.i, os.i);
    TEST_ASSERT_EQUAL_DOUBLE(s.d, os.d);
    TEST_ASSERT_EQUAL((int)s.e, (int)os.e);
    TEST_ASSERT_EQUAL(der.b, oder.b);
    TEST_ASSERT_EQUAL_DOUBLE(der.x, oder.x);
    TEST_ASSERT_EQUAL((int)der.e, (int)oder.e);
  }

  {
    Archive archive{MemoryBuffer{}};
    archive.buffer().fail_on_write_call = 2;
    std::pair<int32_t, int32_t> value{1, 2};
    auto result = archive.Save(value);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::write_error.error_code,
                      result.error().error_code);
    TEST_ASSERT_EQUAL_STRING(ae::seri::write_error.message.data(),
                             result.error().message.data());
    TEST_ASSERT_EQUAL(2, archive.buffer().write_calls);
  }

  {
    Archive archive{MemoryBuffer{}};
    archive.buffer().fail_on_read_call = 2;
    auto const raw11 = raw_bytes(std::uint32_t{11});
    auto const raw22 = raw_bytes(std::uint32_t{22});
    auto const raw33 = raw_bytes(std::uint32_t{33});
    archive.buffer().bytes = raw11;
    archive.buffer().bytes.insert(archive.buffer().bytes.end(), raw22.begin(),
                                  raw22.end());
    archive.buffer().bytes.insert(archive.buffer().bytes.end(), raw33.begin(),
                                  raw33.end());
    std::pair<std::uint32_t, std::uint32_t> value{};
    auto result = archive.Load(value);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::read_error.error_code,
                      result.error().error_code);
    TEST_ASSERT_EQUAL(2, archive.buffer().read_calls);
  }

  {
    Archive archive{MemoryBuffer{}};
    int32_t saved_value = 12;
    auto save_meta =
        ae::seri::Meta{static_cast<int32_t const&>(saved_value), "named"};
    TEST_ASSERT_TRUE(archive.Save(save_meta));
    int32_t loaded_value{};
    auto load_meta = ae::seri::Meta{loaded_value, "named"};
    TEST_ASSERT_TRUE(archive.Load(load_meta));
    TEST_ASSERT_EQUAL(saved_value, loaded_value);
  }

  {
    Archive archive{MemoryBuffer{}};
    int32_t v = 9;
    archive.buffer().write_limit = sizeof(v) - 1;
    auto result = archive.Save(v);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::write_eof.error_code,
                      result.error().error_code);
  }

  {
    Archive archive{MemoryBuffer{}};
    archive.buffer().bytes = {std::byte{0x01}};
    std::uint16_t v{};
    auto result = archive.Load(v);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::read_eof.error_code, result.error().error_code);
  }

  {
    Archive archive{MemoryBuffer{}};
    archive.buffer().fail_on_read_call = 1;
    std::uint32_t v{};
    auto result = archive.Load(v);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::read_error.error_code,
                      result.error().error_code);
    TEST_ASSERT_EQUAL(1, archive.buffer().read_calls);
  }

  {
    Archive archive{MemoryBuffer{}};
    archive.buffer().fail_on_write_call = 1;
    std::uint32_t v{77};
    auto result = archive.Save(v);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::write_error.error_code,
                      result.error().error_code);
    TEST_ASSERT_EQUAL(1, archive.buffer().write_calls);
  }

  {
    Archive archive{MemoryBuffer{}};
    std::vector<std::uint32_t> values{1, 2, 3};
    TEST_ASSERT_TRUE(archive.Save(values));
    TEST_ASSERT_EQUAL(2, archive.buffer().write_calls);
    TEST_ASSERT_EQUAL(1, archive.buffer().size_write_calls);
    TEST_ASSERT_EQUAL(1, archive.buffer().data_write_calls);
    archive.buffer().read_pos = 0;
    std::vector<std::uint32_t> loaded;
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_TRUE(values == loaded);
    TEST_ASSERT_EQUAL(2, archive.buffer().read_calls);
    TEST_ASSERT_EQUAL(1, archive.buffer().size_read_calls);
    TEST_ASSERT_EQUAL(1, archive.buffer().data_read_calls);
  }

  {
    Archive archive{MemoryBuffer{}};
    std::vector<std::uint32_t> values{};
    TEST_ASSERT_TRUE(archive.Save(values));
    archive.buffer().read_pos = 0;
    std::vector<std::uint32_t> loaded{9};
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_TRUE(loaded.empty());
  }

  {
    Archive archive{MemoryBuffer{}};
    std::vector<bool> values{true, false, true};
    TEST_ASSERT_TRUE(archive.Save(values));
    TEST_ASSERT_EQUAL(1 + static_cast<int>(values.size()),
                      archive.buffer().write_calls);
    archive.buffer().read_pos = 0;
    std::vector<bool> loaded;
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_TRUE(values == loaded);
  }

  {
    Archive archive{MemoryBuffer{}};
    std::string text{"abc"};
    TEST_ASSERT_TRUE(archive.Save(text));
    TEST_ASSERT_EQUAL(2, archive.buffer().write_calls);
    archive.buffer().read_pos = 0;
    std::string loaded;
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL_STRING(text.c_str(), loaded.c_str());
    TEST_ASSERT_EQUAL(2, archive.buffer().read_calls);
  }

  {
    Archive archive{MemoryBuffer{}};
    std::string text{};
    TEST_ASSERT_TRUE(archive.Save(text));
    archive.buffer().read_pos = 0;
    std::string loaded{"x"};
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_TRUE(loaded.empty());
  }

  {
    Archive archive{MemoryBuffer{}};
    bool t = true;
    bool f = false;
    TEST_ASSERT_TRUE(archive.Save(t));
    TEST_ASSERT_TRUE(archive.Save(f));
    TEST_ASSERT_EQUAL(2, archive.buffer().bytes.size());
    TEST_ASSERT_EQUAL_UINT8(
        0x01, std::to_integer<std::uint8_t>(archive.buffer().bytes[0]));
    TEST_ASSERT_EQUAL_UINT8(
        0x00, std::to_integer<std::uint8_t>(archive.buffer().bytes[1]));

    archive.buffer().read_pos = 0;
    bool lt{};
    bool lf{true};
    TEST_ASSERT_TRUE(archive.Load(lt));
    TEST_ASSERT_TRUE(archive.Load(lf));
    TEST_ASSERT_TRUE(lt);
    TEST_ASSERT_FALSE(lf);
    TEST_ASSERT_EQUAL(2, archive.buffer().read_calls);
  }

  {
    Archive archive{MemoryBuffer{}};
    archive.buffer().bytes = {std::byte{0x02}};
    bool loaded{true};
    auto result = archive.Load(loaded);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::invalid_bool.error_code,
                      result.error().error_code);
    TEST_ASSERT_TRUE(loaded);
    TEST_ASSERT_EQUAL(1, archive.buffer().read_calls);
  }

  {
    Archive archive{MemoryBuffer{}, 2};
    auto const size = std::size_t{3};
    auto const raw_size = raw_bytes(size);
    archive.buffer().bytes = raw_size;
    std::vector<std::uint32_t> loaded{9};
    auto result = archive.Load(loaded);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::container_too_large.error_code,
                      result.error().error_code);
    TEST_ASSERT_EQUAL(1, archive.buffer().read_calls);
    TEST_ASSERT_EQUAL(1u, loaded.size());
    TEST_ASSERT_EQUAL(9u, loaded[0]);
  }

  {
    Archive archive{MemoryBuffer{}, 8};
    auto const size = std::size_t{2};
    auto const raw_size = raw_bytes(size);
    auto const raw_value0 = raw_bytes(std::uint32_t{11});
    auto const raw_value1 = raw_bytes(std::uint32_t{22});
    archive.buffer().bytes = raw_size;
    archive.buffer().bytes.insert(archive.buffer().bytes.end(),
                                  raw_value0.begin(), raw_value0.end());
    archive.buffer().bytes.insert(archive.buffer().bytes.end(),
                                  raw_value1.begin(), raw_value1.end());
    std::vector<std::uint32_t> loaded;
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL(2u, loaded.size());
    TEST_ASSERT_EQUAL(11u, loaded[0]);
    TEST_ASSERT_EQUAL(22u, loaded[1]);
  }

  {
    Archive archive{MemoryBuffer{}, 4};
    auto const size = std::size_t{2};
    auto const raw_size = raw_bytes(size);
    auto const raw_value0 = raw_bytes(std::uint32_t{11});
    auto const raw_value1 = raw_bytes(std::uint32_t{22});
    archive.buffer().bytes = raw_size;
    archive.buffer().bytes.insert(archive.buffer().bytes.end(),
                                  raw_value0.begin(), raw_value0.end());
    archive.buffer().bytes.insert(archive.buffer().bytes.end(),
                                  raw_value1.begin(), raw_value1.end());
    std::vector<std::uint32_t> loaded{9};
    auto result = archive.Load(loaded);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::container_too_large.error_code,
                      result.error().error_code);
    TEST_ASSERT_EQUAL(1, archive.buffer().read_calls);
    TEST_ASSERT_EQUAL(1u, loaded.size());
    TEST_ASSERT_EQUAL(9u, loaded[0]);
  }

  {
    Archive archive{MemoryBuffer{}, 2};
    auto const size = std::size_t{3};
    auto const raw_size = raw_bytes(size);
    archive.buffer().bytes = raw_size;
    std::vector<bool> loaded{true};
    auto result = archive.Load(loaded);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::container_too_large.error_code,
                      result.error().error_code);
    TEST_ASSERT_EQUAL(1, archive.buffer().read_calls);
    TEST_ASSERT_TRUE(loaded[0]);
  }

  {
    Archive archive{MemoryBuffer{}, 3};
    auto const size = std::size_t{3};
    auto const raw_size = raw_bytes(size);
    archive.buffer().bytes = raw_size;
    archive.buffer().bytes.push_back(std::byte{1});
    archive.buffer().bytes.push_back(std::byte{0});
    archive.buffer().bytes.push_back(std::byte{1});
    std::vector<bool> loaded;
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL(3u, loaded.size());
    TEST_ASSERT_TRUE(loaded[0]);
    TEST_ASSERT_FALSE(loaded[1]);
    TEST_ASSERT_TRUE(loaded[2]);
    TEST_ASSERT_EQUAL(4, archive.buffer().read_calls);
  }

  {
    Archive archive{MemoryBuffer{}, 2};
    auto const size = std::size_t{3};
    auto const raw_size = raw_bytes(size);
    archive.buffer().bytes = raw_size;
    std::string loaded{"x"};
    auto result = archive.Load(loaded);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ae::seri::container_too_large.error_code,
                      result.error().error_code);
    TEST_ASSERT_EQUAL(1, archive.buffer().read_calls);
    TEST_ASSERT_EQUAL_STRING("x", loaded.c_str());
  }

  {
    Archive archive{MemoryBuffer{}};
    std::vector<TestEnum> values{TestEnum::A, TestEnum::B, TestEnum::A};
    TEST_ASSERT_TRUE(archive.Save(values));
    TEST_ASSERT_EQUAL(2, archive.buffer().write_calls);
    TEST_ASSERT_EQUAL(1, archive.buffer().size_write_calls);
    TEST_ASSERT_EQUAL(1, archive.buffer().data_write_calls);
    archive.buffer().read_pos = 0;
    std::vector<TestEnum> loaded;
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL(2, archive.buffer().read_calls);
    TEST_ASSERT_EQUAL(1, archive.buffer().size_read_calls);
    TEST_ASSERT_EQUAL(1, archive.buffer().data_read_calls);
    TEST_ASSERT_TRUE(values == loaded);
  }

  {
    Archive archive{MemoryBuffer{}};
    std::array<std::uint32_t, 3> values{1, 2, 3};
    TEST_ASSERT_TRUE(archive.Save(values));
    TEST_ASSERT_EQUAL(1, archive.buffer().data_write_calls);
    TEST_ASSERT_EQUAL(0, archive.buffer().size_write_calls);
    archive.buffer().read_pos = 0;
    std::array<std::uint32_t, 3> loaded{};
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL(1, archive.buffer().data_read_calls);
    TEST_ASSERT_EQUAL(0, archive.buffer().size_read_calls);
    TEST_ASSERT_TRUE(values == loaded);
  }

  {
    Archive archive{MemoryBuffer{}};
    std::array<std::uint32_t, 0> values{};
    TEST_ASSERT_TRUE(archive.Save(values));
    TEST_ASSERT_EQUAL(0, archive.buffer().data_write_calls);
    TEST_ASSERT_EQUAL(0, archive.buffer().size_write_calls);
    std::array<std::uint32_t, 0> loaded{};
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL(0, archive.buffer().data_read_calls);
    TEST_ASSERT_EQUAL(0, archive.buffer().size_read_calls);
  }

  {
    Archive archive{MemoryBuffer{}};
    std::array<Simple, 2> values{
        {{1, 2.0, TestEnum::A}, {3, 4.0, TestEnum::B}}};
    TEST_ASSERT_TRUE(archive.Save(values));
    TEST_ASSERT_EQUAL(6, archive.buffer().data_write_calls);
    TEST_ASSERT_TRUE(archive.buffer().write_calls > 1);
    archive.buffer().read_pos = 0;
    std::array<Simple, 2> loaded{};
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL(6, archive.buffer().data_read_calls);
    TEST_ASSERT_TRUE(values[0].i == loaded[0].i);
  }

  {
    Archive archive{MemoryBuffer{}, 3};
    auto const size = std::size_t{3};
    auto const raw_size = raw_bytes(size);
    archive.buffer().bytes = raw_size;
    archive.buffer().bytes.push_back(std::byte{'a'});
    archive.buffer().bytes.push_back(std::byte{'b'});
    archive.buffer().bytes.push_back(std::byte{'c'});
    std::string loaded;
    TEST_ASSERT_TRUE(archive.Load(loaded));
    TEST_ASSERT_EQUAL_STRING("abc", loaded.c_str());
  }
}
}  // namespace test_binary_archive

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
#include <chrono>
#include <cstddef>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <unity.h>

#include "aether-miscpp/serialization/binary_archive.h"
#include "aether-miscpp/serialization/details/member_serializer.h"

namespace test_binary_std_serializers {
template <typename T>
struct TestAllocator {
  using value_type = T;
  TestAllocator() = default;
  template <typename U>
  TestAllocator(TestAllocator<U> const&) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
};

struct CustomCompare {
  bool operator()(int lhs, int rhs) const { return lhs > rhs; }
};

struct NonAssignable {
  NonAssignable() = default;
  explicit NonAssignable(int v) : value(v) {}
  NonAssignable(NonAssignable const&) = default;
  NonAssignable(NonAssignable&&) noexcept = default;
  NonAssignable& operator=(NonAssignable const&) = delete;
  NonAssignable& operator=(NonAssignable&&) = delete;
  int value{};

  template <typename Archive>
  ae::seri::SeriResult Seri(Archive& archive) const {
    return archive.Save(value);
  }

  template <typename Archive>
  ae::seri::SeriResult Deseri(Archive& archive) {
    return archive.Load(value);
  }
};

struct PairRoutingKey {
  int value{};

  friend bool operator==(PairRoutingKey const& lhs, PairRoutingKey const& rhs) {
    return lhs.value == rhs.value;
  }

  friend bool operator<(PairRoutingKey const& lhs, PairRoutingKey const& rhs) {
    return lhs.value < rhs.value;
  }
};

struct PairRoutingValue {
  int value{};
};

struct PairRoutingHash {
  std::size_t operator()(PairRoutingKey const& key) const {
    return std::hash<int>{}(key.value);
  }
};

inline int pair_save_count{};
inline int pair_load_count{};

struct PlainArchiveBuffer {
  std::vector<std::byte> bytes;
  std::size_t read_pos{};

  ae::seri::SeriResult Write(ae::seri::SizeWriteTag tag) {
    auto const* ptr = reinterpret_cast<std::byte const*>(&tag.size);
    bytes.insert(bytes.end(), ptr, ptr + sizeof(tag.size));
    return ae::Ok{ae::seri::good};
  }

  ae::seri::SeriResult Write(ae::seri::DataWriteTag tag) {
    auto const* ptr = reinterpret_cast<std::byte const*>(tag.data);
    bytes.insert(bytes.end(), ptr, ptr + tag.size);
    return ae::Ok{ae::seri::good};
  }

  ae::seri::SeriResult Read(ae::seri::SizeReadTag tag) {
    std::memcpy(&tag.size, bytes.data() + read_pos, sizeof(tag.size));
    read_pos += sizeof(tag.size);
    return ae::Ok{ae::seri::good};
  }

  ae::seri::SeriResult Read(ae::seri::DataReadTag tag) {
    std::memcpy(tag.data, bytes.data() + read_pos, tag.size);
    read_pos += tag.size;
    return ae::Ok{ae::seri::good};
  }
};
}  // namespace test_binary_std_serializers

namespace ae::seri {
// Instruments pair serialization to verify map entries route through it.
template <typename B>
struct Serializer<BinaryArchive<B>,
                  std::pair<const test_binary_std_serializers::PairRoutingKey,
                            test_binary_std_serializers::PairRoutingValue>> {
  SeriResult Seri(
      BinaryArchive<B>& archive,
      Meta<std::pair<const test_binary_std_serializers::PairRoutingKey,
                     test_binary_std_serializers::PairRoutingValue> const>
          val) {
    ++test_binary_std_serializers::pair_save_count;
    TRY_RESULT(archive.Save(val.value.first.value));
    return archive.Save(val.value.second.value);
  }
};

template <typename B>
struct Serializer<BinaryArchive<B>,
                  std::pair<test_binary_std_serializers::PairRoutingKey,
                            test_binary_std_serializers::PairRoutingValue>> {
  SeriResult Deseri(
      BinaryArchive<B>& archive,
      Meta<std::pair<test_binary_std_serializers::PairRoutingKey,
                     test_binary_std_serializers::PairRoutingValue>>
          val) {
    ++test_binary_std_serializers::pair_load_count;
    TRY_RESULT(archive.Load(val.value.first.value));
    return archive.Load(val.value.second.value);
  }
};
}  // namespace ae::seri

namespace test_binary_std_serializers {
// Covers BinaryArchive std serializers and container caps.
void test_BinaryStdSerializers() {
  using Buffer = PlainArchiveBuffer;
  using Archive = ae::seri::BinaryArchive<Buffer>;

  Buffer buffer{};
  Archive archive{std::move(buffer), 8};

  std::vector<int> vec{1, 2, 3};
  std::string str{"abc"};
  std::list<int> lst{4, 5};
  std::deque<int> deq{6, 7};
  std::set<int> set{8, 9};
  std::multiset<int> mset{10, 10};
  std::unordered_set<int> uset{11, 12};

  std::map<int, std::string> mp{{1, "one"}, {2, "two"}};
  std::multimap<int, std::string> mmp{{3, "three"}, {3, "trois"}};
  std::unordered_map<int, std::string> ump{{4, "four"}, {5, "five"}};
  std::unordered_multimap<int, std::string> ummp{{6, "six"}, {6, "sechs"}};

  std::map<PairRoutingKey, PairRoutingValue> routed_map{{{1}, {10}},
                                                        {{2}, {20}}};
  std::multimap<PairRoutingKey, PairRoutingValue> routed_multimap{{{3}, {30}},
                                                                  {{3}, {31}}};
  std::unordered_map<PairRoutingKey, PairRoutingValue, PairRoutingHash>
      routed_unordered_map{{{4}, {40}}, {{5}, {50}}};
  std::unordered_multimap<PairRoutingKey, PairRoutingValue, PairRoutingHash>
      routed_unordered_multimap{{{6}, {60}}, {{6}, {61}}};

  std::optional<int> opt{42};
  std::pair<int, std::string> pr{7, "pair"};
  std::array<int, 2> arr{{8, 9}};
  std::tuple<> empty_tup{};
  std::tuple<int, std::string, bool> tup{10, "tuple", true};
  std::variant<int, std::string> var{std::in_place_index<1>, "variant"};
  auto dur = std::chrono::milliseconds{123};
  auto tp = std::chrono::time_point<std::chrono::system_clock,
                                    std::chrono::milliseconds>{dur};

  TEST_ASSERT_TRUE(archive.Save(vec).IsOk());
  TEST_ASSERT_TRUE(archive.Save(str).IsOk());
  TEST_ASSERT_TRUE(archive.Save(lst).IsOk());
  TEST_ASSERT_TRUE(archive.Save(deq).IsOk());
  TEST_ASSERT_TRUE(archive.Save(set).IsOk());
  TEST_ASSERT_TRUE(archive.Save(mset).IsOk());
  TEST_ASSERT_TRUE(archive.Save(uset).IsOk());
  TEST_ASSERT_TRUE(archive.Save(mp).IsOk());
  TEST_ASSERT_TRUE(archive.Save(mmp).IsOk());
  TEST_ASSERT_TRUE(archive.Save(ump).IsOk());
  TEST_ASSERT_TRUE(archive.Save(ummp).IsOk());
  TEST_ASSERT_TRUE(archive.Save(routed_map).IsOk());
  TEST_ASSERT_TRUE(archive.Save(routed_multimap).IsOk());
  TEST_ASSERT_TRUE(archive.Save(routed_unordered_map).IsOk());
  TEST_ASSERT_TRUE(archive.Save(routed_unordered_multimap).IsOk());
  TEST_ASSERT_EQUAL_INT(8, pair_save_count);
  TEST_ASSERT_TRUE(archive.Save(opt).IsOk());
  TEST_ASSERT_TRUE(archive.Save(pr).IsOk());
  TEST_ASSERT_TRUE(archive.Save(arr).IsOk());
  auto const empty_tup_bytes_before = buffer.bytes.size();
  TEST_ASSERT_TRUE(archive.Save(empty_tup).IsOk());
  TEST_ASSERT_EQUAL(empty_tup_bytes_before, buffer.bytes.size());
  TEST_ASSERT_TRUE(archive.Save(tup).IsOk());
  TEST_ASSERT_TRUE(archive.Save(var).IsOk());
  TEST_ASSERT_TRUE(archive.Save(dur).IsOk());
  TEST_ASSERT_TRUE(archive.Save(tp).IsOk());

  std::set<int, CustomCompare, TestAllocator<int>> custom_set{3, 1, 2};
  std::optional<NonAssignable> opt_na{std::in_place, 11};
  std::variant<int, NonAssignable> var_na{std::in_place_index<1>, 22};
  TEST_ASSERT_TRUE(archive.Save(custom_set).IsOk());
  TEST_ASSERT_TRUE(archive.Save(opt_na).IsOk());
  TEST_ASSERT_TRUE(archive.Save(var_na).IsOk());

  auto loaded = Archive{std::move(archive).buffer()};
  std::vector<int> vec2;
  std::string str2;
  std::list<int> lst2;
  std::deque<int> deq2;
  std::set<int> set2;
  std::multiset<int> mset2;
  std::unordered_set<int> uset2;
  std::map<int, std::string> mp2;
  std::multimap<int, std::string> mmp2;
  std::unordered_map<int, std::string> ump2;
  std::unordered_multimap<int, std::string> ummp2;
  std::map<PairRoutingKey, PairRoutingValue> routed_map2;
  std::multimap<PairRoutingKey, PairRoutingValue> routed_multimap2;
  std::unordered_map<PairRoutingKey, PairRoutingValue, PairRoutingHash>
      routed_unordered_map2;
  std::unordered_multimap<PairRoutingKey, PairRoutingValue, PairRoutingHash>
      routed_unordered_multimap2;
  std::optional<int> opt2;
  std::pair<int, std::string> pr2;
  std::array<int, 2> arr2{};
  std::tuple<> empty_tup2{};
  std::tuple<int, std::string, bool> tup2{};
  std::variant<int, std::string> var2;
  std::chrono::milliseconds dur2{};
  std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
      tp2{};

  TEST_ASSERT_TRUE(loaded.Load(vec2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(str2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(lst2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(deq2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(set2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(mset2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(uset2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(mp2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(mmp2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(ump2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(ummp2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(routed_map2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(routed_multimap2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(routed_unordered_map2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(routed_unordered_multimap2).IsOk());
  TEST_ASSERT_EQUAL_INT(8, pair_load_count);
  TEST_ASSERT_EQUAL_INT(10, routed_map2.at(PairRoutingKey{1}).value);
  TEST_ASSERT_EQUAL_INT(20, routed_map2.at(PairRoutingKey{2}).value);
  TEST_ASSERT_EQUAL_INT(2, static_cast<int>(routed_multimap2.size()));
  TEST_ASSERT_EQUAL_INT(40, routed_unordered_map2.at(PairRoutingKey{4}).value);
  TEST_ASSERT_EQUAL_INT(50, routed_unordered_map2.at(PairRoutingKey{5}).value);
  TEST_ASSERT_EQUAL_INT(2, static_cast<int>(routed_unordered_multimap2.size()));
  TEST_ASSERT_TRUE(loaded.Load(opt2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(pr2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(arr2).IsOk());
  auto const loaded_bytes_before_empty_tup = loaded.buffer().read_pos;
  TEST_ASSERT_TRUE(loaded.Load(empty_tup2).IsOk());
  TEST_ASSERT_EQUAL(loaded_bytes_before_empty_tup, loaded.buffer().read_pos);
  TEST_ASSERT_TRUE(loaded.Load(tup2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(var2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(dur2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(tp2).IsOk());

  std::set<int, CustomCompare, TestAllocator<int>> custom_set2;
  std::optional<NonAssignable> opt_na2;
  std::variant<int, NonAssignable> var_na2;
  TEST_ASSERT_TRUE(loaded.Load(custom_set2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(opt_na2).IsOk());
  TEST_ASSERT_TRUE(loaded.Load(var_na2).IsOk());

  TEST_ASSERT_EQUAL_INT(3, static_cast<int>(vec2.size()));
  TEST_ASSERT_EQUAL_STRING("abc", str2.c_str());
  TEST_ASSERT_TRUE(opt2.has_value());
  TEST_ASSERT_EQUAL_STRING("variant", std::get<std::string>(var2).c_str());
  TEST_ASSERT_EQUAL_INT(123, dur2.count());
  TEST_ASSERT_EQUAL_INT(123, tp2.time_since_epoch().count());
  {
    Archive chrono_archive{Buffer{}, 8};
    std::chrono::seconds secs{2};
    TEST_ASSERT_TRUE(chrono_archive.Save(secs).IsOk());
    TEST_ASSERT_EQUAL(sizeof(std::uint64_t),
                      chrono_archive.buffer().bytes.size());
    std::uint64_t saved_ms_count{};
    std::memcpy(&saved_ms_count, chrono_archive.buffer().bytes.data(),
                sizeof(saved_ms_count));
    TEST_ASSERT_EQUAL_UINT(2000, saved_ms_count);

    auto const tp_sec =
        std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::seconds>{std::chrono::seconds{3}};
    TEST_ASSERT_TRUE(chrono_archive.Save(tp_sec).IsOk());
    std::uint64_t saved_tp_ms_count{};
    std::memcpy(&saved_tp_ms_count,
                chrono_archive.buffer().bytes.data() + sizeof(std::uint64_t),
                sizeof(saved_tp_ms_count));
    TEST_ASSERT_EQUAL_UINT(3000, saved_tp_ms_count);

    chrono_archive.buffer().read_pos = 0;
    std::chrono::seconds loaded_secs{};
    TEST_ASSERT_TRUE(chrono_archive.Load(loaded_secs).IsOk());
    TEST_ASSERT_EQUAL_INT(2, loaded_secs.count());
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
        loaded_tp{};
    TEST_ASSERT_TRUE(chrono_archive.Load(loaded_tp).IsOk());
    TEST_ASSERT_EQUAL_INT(3, loaded_tp.time_since_epoch().count());

    Archive chrono_load_archive{Buffer{}, 8};
    std::uint64_t raw_2500_value{2500};
    auto const* raw_2500 = reinterpret_cast<std::byte const*>(&raw_2500_value);
    chrono_load_archive.buffer().bytes.insert(
        chrono_load_archive.buffer().bytes.end(), raw_2500,
        raw_2500 + sizeof(std::uint64_t));
    std::chrono::seconds loaded_secs_2500{};
    TEST_ASSERT_TRUE(chrono_load_archive.Load(loaded_secs_2500).IsOk());
    TEST_ASSERT_EQUAL_INT(2, loaded_secs_2500.count());

    std::uint64_t raw_3500_value{3500};
    auto const* raw_3500 = reinterpret_cast<std::byte const*>(&raw_3500_value);
    chrono_load_archive.buffer().bytes.insert(
        chrono_load_archive.buffer().bytes.end(), raw_3500,
        raw_3500 + sizeof(std::uint64_t));
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
        loaded_tp_3500{};
    TEST_ASSERT_TRUE(chrono_load_archive.Load(loaded_tp_3500).IsOk());
    TEST_ASSERT_EQUAL_INT(3, loaded_tp_3500.time_since_epoch().count());
  }
  TEST_ASSERT_TRUE(opt_na2.has_value());
  TEST_ASSERT_EQUAL(11, opt_na2->value);
  TEST_ASSERT_EQUAL(22, std::get<NonAssignable>(var_na2).value);

  Archive invalid_variant_archive{Buffer{}, 8};
  TEST_ASSERT_TRUE(invalid_variant_archive.Save(std::uint8_t{99}).IsOk());
  auto invalid_variant_loaded =
      Archive{std::move(invalid_variant_archive).buffer()};
  std::variant<int, std::string> invalid_variant{};
  TEST_ASSERT_FALSE(invalid_variant_loaded.Load(invalid_variant).IsOk());

  Archive variant_width_archive{Buffer{}, 8};
  std::variant<int, std::string> width_variant{std::in_place_index<1>, "wide"};
  TEST_ASSERT_TRUE(variant_width_archive.Save(width_variant).IsOk());
  TEST_ASSERT_EQUAL_UINT8(0x01, std::to_integer<std::uint8_t>(
                                    variant_width_archive.buffer().bytes[0]));

  Archive cap_archive{Buffer{}, 1};
  TEST_ASSERT_TRUE(cap_archive.Save(std::size_t{2}).IsOk());
  auto cap_loaded = Archive{std::move(cap_archive).buffer(), 1};
  std::map<int, int> too_big;
  TEST_ASSERT_FALSE(cap_loaded.Load(too_big).IsOk());
}
}  // namespace test_binary_std_serializers

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

#include <string_view>
#include <type_traits>
#include <utility>

#include <unity.h>

#include "aether-miscpp/serialization/details/meta.h"
#include "aether-miscpp/serialization/details/serializer.h"

namespace test_archive_facade {

struct NamedArchive {
  int saved_value{};
  std::string_view saved_name{};

  template <typename T>
  ae::seri::SeriResult Save(ae::seri::Meta<T const> const& meta) {
    return ae::seri::Serializer<NamedArchive, T>{}.Seri(*this, meta);
  }

  template <typename T>
    requires(!ae::seri::IsMetaType_v<T>)
  ae::seri::SeriResult Save(T const& value) {
    return Save(ae::seri::Meta{value, ""});
  }

  template <typename T>
  ae::seri::SeriResult Load(ae::seri::Meta<T> meta) {
    return ae::seri::Serializer<NamedArchive, T>{}.Deseri(*this, meta);
  }

  template <typename T>
    requires(!ae::seri::IsMetaType_v<T>)
  ae::seri::SeriResult Load(T& value) {
    return Load(ae::seri::Meta{value, ""});
  }
};

struct MultiArchive {
  int saves{};
  int loads{};

  template <typename T>
  ae::Result<ae::seri::Good, ae::seri::SeriError> Save(
      ae::seri::Meta<T const> const& meta) {
    return ae::seri::Serializer<MultiArchive, T>{}.Seri(*this, meta);
  }

  template <typename T>
    requires(!ae::seri::IsMetaType_v<T>)
  ae::Result<ae::seri::Good, ae::seri::SeriError> Save(T const& value) {
    return Save(ae::seri::Meta{value, ""});
  }

  template <typename T>
  ae::Result<ae::seri::Good, ae::seri::SeriError> Load(ae::seri::Meta<T> meta) {
    return ae::seri::Serializer<MultiArchive, T>{}.Deseri(*this, meta);
  }

  template <typename T>
    requires(!ae::seri::IsMetaType_v<T>)
  ae::Result<ae::seri::Good, ae::seri::SeriError> Load(T& value) {
    return Load(ae::seri::Meta{value, ""});
  }
};

}  // namespace test_archive_facade

namespace ae::seri {
template <>
struct Serializer<test_archive_facade::NamedArchive, int> {
  SeriResult Seri(test_archive_facade::NamedArchive& archive,
                  Meta<int const> const& value) {
    archive.saved_value = value.value;
    archive.saved_name = value.name;
    return Ok{good};
  }
  SeriResult Deseri(test_archive_facade::NamedArchive&, Meta<int>) {
    return Ok{good};
  }
};

template <>
struct Serializer<test_archive_facade::MultiArchive, int> {
  SeriResult Seri(test_archive_facade::MultiArchive& archive, Meta<int const>) {
    ++archive.saves;
    return Ok{good};
  }
  SeriResult Deseri(test_archive_facade::MultiArchive& archive, Meta<int>) {
    ++archive.loads;
    return Ok{good};
  }
};
}  // namespace ae::seri

namespace test_archive_facade {
// Verifies archive member facade forwarding with single-value operations.
void test_ArchiveFacade() {
  NamedArchive archive{};
  int value = 42;
  TEST_ASSERT_TRUE((
      archive.Save(ae::seri::Meta<int const>{std::as_const(value), "answer"})));
  TEST_ASSERT_EQUAL(42, archive.saved_value);
  TEST_ASSERT_EQUAL_STRING("answer", archive.saved_name.data());
  TEST_ASSERT_EQUAL(42, value);
  MultiArchive multi{};
  int a = 1;
  int b = 2;
  TEST_ASSERT_TRUE(multi.Save(a));
  TEST_ASSERT_TRUE(multi.Save(b));
  TEST_ASSERT_EQUAL(2, multi.saves);
  TEST_ASSERT_TRUE(multi.Load(a));
  TEST_ASSERT_TRUE(multi.Load(b));
  TEST_ASSERT_EQUAL(2, multi.loads);
}
}  // namespace test_archive_facade

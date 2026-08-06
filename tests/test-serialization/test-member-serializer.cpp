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

#include <type_traits>
#include <vector>

#include <utility>

#include <unity.h>

#include "aether-miscpp/reflect/reflect.h"
#include "aether-miscpp/serialization/details/member_serializer.h"
#include "aether-miscpp/serialization/details/reflectable_serializer.h"

namespace test_member_serializer {
struct MemberArchive {
  std::vector<int> saved_values{};
  std::vector<int> loaded_values{};
  std::size_t load_index{};
  int member_saves{};
  int member_loads{};
  int explicit_saves{};
  int explicit_loads{};

  template <typename T>
    requires(ae::seri::IsMetaType_v<T>)
  ae::seri::SeriResult Save(T meta) {
    using value_type = std::remove_const_t<ae::seri::MetaValueType_t<T>>;
    return ae::seri::Serializer<MemberArchive, value_type>{}.Seri(
        *this, ae::seri::Meta<value_type const>{meta.value, meta.name});
  }

  template <typename T>
    requires(!ae::seri::IsMetaType_v<T>)
  ae::seri::SeriResult Save(T const& value) {
    return Save(ae::seri::Meta{value, ""});
  }

  template <typename T>
    requires(ae::seri::IsMetaType_v<T>)
  ae::seri::SeriResult Load(T meta) {
    using value_type = ae::seri::MetaValueType_t<T>;
    return ae::seri::Serializer<MemberArchive, value_type>{}.Deseri(
        *this, ae::seri::Meta<value_type>{meta.value, meta.name});
  }

  template <typename T>
    requires(!ae::seri::IsMetaType_v<T>)
  ae::seri::SeriResult Load(T& value) {
    return Load(ae::seri::Meta{value, ""});
  }
};

template <typename T>
struct Bar {
  T value{};

  ae::seri::SeriResult Seri(MemberArchive& archive) const {
    ++archive.member_saves;
    archive.saved_values.push_back(static_cast<int>(value));
    return ae::Ok{ae::seri::good};
  }

  ae::seri::SeriResult Deseri(MemberArchive& archive) {
    ++archive.member_loads;
    value = static_cast<T>(archive.loaded_values.at(archive.load_index++));
    return ae::Ok{ae::seri::good};
  }
};

struct Foo : public Bar<int> {};

struct ReflectedMemberType {
  int value{};
  mutable int member_calls{};

  AE_REFLECT_MEMBERS(value);

  ae::seri::SeriResult Seri(MemberArchive& archive) const {
    ++archive.member_saves;
    ++member_calls;
    archive.saved_values.push_back(value + 100);
    return ae::Ok{ae::seri::good};
  }

  ae::seri::SeriResult Deseri(MemberArchive& archive) {
    ++archive.member_loads;
    ++member_calls;
    value = archive.loaded_values.at(archive.load_index++) - 100;
    return ae::Ok{ae::seri::good};
  }
};

struct ExplicitWinsType {
  int value{};
  mutable int member_calls{};

  ae::seri::SeriResult Seri(MemberArchive& archive) const {
    ++archive.member_saves;
    ++member_calls;
    archive.saved_values.push_back(value + 1);
    return ae::Ok{ae::seri::good};
  }

  ae::seri::SeriResult Deseri(MemberArchive& archive) {
    ++archive.member_loads;
    ++member_calls;
    value = archive.loaded_values.at(archive.load_index++) - 1;
    return ae::Ok{ae::seri::good};
  }
};

}  // namespace test_member_serializer

namespace ae::seri {
template <>
struct Serializer<test_member_serializer::MemberArchive,
                  test_member_serializer::ExplicitWinsType> {
  SeriResult Deseri(test_member_serializer::MemberArchive& archive,
                    Meta<test_member_serializer::ExplicitWinsType> val) {
    ++archive.explicit_loads;
    val.value.value = archive.loaded_values.at(archive.load_index++) - 10;
    return Ok{good};
  }

  SeriResult Seri(
      test_member_serializer::MemberArchive& archive,
      Meta<test_member_serializer::ExplicitWinsType const> const& val) {
    ++archive.explicit_saves;
    archive.saved_values.push_back(val.value.value + 10);
    return Ok{good};
  }
};
}  // namespace ae::seri

namespace test_member_serializer {
// Covers member fallback, inherited member lookup, reflectable exclusion, and
// explicit serializer precedence.
void test_MemberSerializer() {
  MemberArchive archive{};

  Foo foo{};
  foo.value = 17;
  TEST_ASSERT_TRUE(archive.Save(foo));
  TEST_ASSERT_EQUAL(1, archive.member_saves);
  TEST_ASSERT_EQUAL(17, archive.saved_values.at(0));
  archive.loaded_values = {23};
  TEST_ASSERT_TRUE(archive.Load(foo));
  TEST_ASSERT_EQUAL(1, archive.member_loads);
  TEST_ASSERT_EQUAL(23, foo.value);

  archive = {};
  ReflectedMemberType reflected{};
  reflected.value = 31;
  TEST_ASSERT_TRUE(archive.Save(reflected));
  TEST_ASSERT_EQUAL(1, archive.member_saves);
  TEST_ASSERT_EQUAL(131, archive.saved_values.at(0));
  archive.loaded_values = {141};
  TEST_ASSERT_TRUE(archive.Load(reflected));
  TEST_ASSERT_EQUAL(1, archive.member_loads);
  TEST_ASSERT_EQUAL(41, reflected.value);
  TEST_ASSERT_EQUAL(2, reflected.member_calls);

  archive = {};
  ExplicitWinsType explicit_type{};
  explicit_type.value = 7;
  TEST_ASSERT_TRUE(archive.Save(explicit_type));
  TEST_ASSERT_EQUAL(1, archive.explicit_saves);
  TEST_ASSERT_EQUAL(0, archive.member_saves);
  TEST_ASSERT_EQUAL(17, archive.saved_values.at(0));
  archive.loaded_values = {29};
  TEST_ASSERT_TRUE(archive.Load(explicit_type));
  TEST_ASSERT_EQUAL(1, archive.explicit_loads);
  TEST_ASSERT_EQUAL(0, archive.member_loads);
  TEST_ASSERT_EQUAL(19, explicit_type.value);
}
}  // namespace test_member_serializer

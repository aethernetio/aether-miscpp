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

#ifndef AETHER_SERIALIZATION_DETAILS_REFLECTABLE_SERIALIZER_H_
#define AETHER_SERIALIZATION_DETAILS_REFLECTABLE_SERIALIZER_H_

#include <type_traits>

#include "aether-miscpp/meta/as_type.h"
#include "aether-miscpp/reflect/reflect.h"
#include "aether-miscpp/serialization/details/member_serializer.h"
#include "aether-miscpp/serialization/details/meta.h"

namespace ae::seri {
namespace reflectable_serializer_internal {
template <typename MetaT, typename Obj>
constexpr auto MakeArchiveMeta(Obj&& obj, MetaT const& refl_meta) {
  using meta_class_type = typename std::decay_t<MetaT>::class_type;
  return Meta{
      refl_meta.get(::ae::as_type<meta_class_type>(std::forward<Obj>(obj))),
      refl_meta.name};
}

template <typename MetaT, typename Obj>
constexpr auto MakeArchiveMetaSave(Obj&& obj, MetaT const& refl_meta) {
  using meta_value_type = typename std::decay_t<MetaT>::value_type;
  using meta_class_type = typename std::decay_t<MetaT>::class_type;

  decltype(auto) val =
      refl_meta.get(::ae::as_type<meta_class_type>(std::forward<Obj>(obj)));
  return Meta<meta_value_type const>{val, refl_meta.name};
}

}  // namespace reflectable_serializer_internal

template <typename Archive, typename T>
  requires(reflect::Reflectable<T> &&
           !member_serializer_internal::MemberSerializable<Archive, T>)
struct Serializer<Archive, T> {
  SeriResult Deseri(Archive& archive, Meta<T> val) {
    auto refl = reflect::make_reflection(val.value);
    if constexpr (std::decay_t<decltype(refl)>::mirror_type::kSize == 0) {
      return Ok{good};
    } else {
      return refl.ApplyMeta([&](auto& obj, auto... refl_meta) {
        SeriResult result = Ok{good};
        auto _ = ((result = archive.Load(
                       reflectable_serializer_internal::MakeArchiveMeta(
                           obj, refl_meta)),
                   !!result) &&
                  ...);
        (void)_;
        return result;
      });
    }
  }

  SeriResult Seri(Archive& archive, Meta<T const> const& val) {
    auto refl = reflect::make_reflection(val.value);
    if constexpr (std::decay_t<decltype(refl)>::mirror_type::kSize == 0) {
      return Ok{good};
    } else {
      return refl.ApplyMeta([&](auto const& obj, auto... refl_meta) {
        SeriResult result = Ok{good};
        auto _ = ((result = archive.Save(
                       reflectable_serializer_internal::MakeArchiveMetaSave(
                           obj, refl_meta)),
                   !!result) &&
                  ...);
        (void)_;
        return result;
      });
    }
  }
};
}  // namespace ae::seri

#endif

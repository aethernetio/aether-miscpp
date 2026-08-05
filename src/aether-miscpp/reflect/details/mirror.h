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

#ifndef AETHER_MISCPP_REFLECT_DETAILS_MIRROR_H_
#define AETHER_MISCPP_REFLECT_DETAILS_MIRROR_H_

#include <functional>
#include <tuple>

#include "aether-miscpp/meta/tag_invoke.h"
#include "aether-miscpp/reflect/details/meta.h"

namespace ae::reflect {
template <typename T>
concept MirrorType = requires(T mirror) {
  typename std::decay_t<T>::meta_list;
  typename std::decay_t<T>::class_type;
  // check all elements are MetaType
  []<std::size_t... I>(std::index_sequence<I...>) {
    return (MetaType<
                std::tuple_element_t<I, typename std::decay_t<T>::meta_list>> &&
            ...);
  }(std::make_index_sequence<
      std::tuple_size_v<typename std::decay_t<T>::meta_list>>());
};

template <typename T>
concept MirrorMeta = requires { requires(MetaType<T> || MirrorType<T>); };

static constexpr auto CatMetaListImpl() { return std::tuple{}; }

template <MetaType T>
static constexpr auto CatMetaListImpl(T&& t) {
  return std::tuple{std::forward<T>(t)};
};

template <MirrorType T>
static constexpr auto CatMetaListImpl(T&& t) {
  return std::forward<T>(t).meta_list_;
};

template <MirrorMeta T, MirrorMeta... Metas>
static constexpr auto CatMetaListImpl(T&& t, Metas&&... ms) {
  if constexpr (MirrorType<std::decay_t<T>>) {
    return std::tuple_cat(std::forward<T>(t).meta_list_,
                          CatMetaListImpl(std::forward<Metas>(ms)...));
  } else {
    return std::tuple_cat(std::tuple{std::forward<T>(t)},
                          CatMetaListImpl(std::forward<Metas>(ms)...));
  }
};

template <MirrorMeta... Metas>
static constexpr auto CatMetaList(Metas&&... ms) {
  return CatMetaListImpl(std::forward<Metas>(ms)...);
};

template <typename From, typename To>
inline constexpr bool IsStaticCovertible =
    std::is_void_v<From> || std::is_void_v<To> ||
    std::is_convertible_v<From&, To&> || std::is_convertible_v<To&, From&>;

template <typename...>
struct MostDerivedImpl;
template <>
struct MostDerivedImpl<> {
  using type = void;
};
template <typename T1, typename T2, typename... T>
struct MostDerivedImpl<T1, T2, T...> {
  using type = std::conditional_t<std::is_base_of_v<T1, T2>,
                                  typename MostDerivedImpl<T2, T...>::type,
                                  typename MostDerivedImpl<T1, T...>::type>;
};
template <typename T>
struct MostDerivedImpl<T> {
  using type = T;
};
template <typename... T>
struct MostDerived {
  using type = typename MostDerivedImpl<T...>::type;
};
template <typename... T>
using MostDerived_t = typename MostDerived<T...>::type;

template <typename T>
struct ClassTag {};

/**
 * \brief List of meta types for class_type
 */
template <typename ClassType, MirrorMeta... Metas>
class Mirror {
 public:
  using class_type = ClassType;
  static_assert((std::is_void_v<class_type> ||
                 (IsStaticCovertible<class_type, typename Metas::class_type> &&
                  ...)),
                "All meta types must have the same class type");

  using meta_list = decltype(CatMetaList(std::declval<Metas>()...));

  template <std::size_t I>
  using meta_at = std::tuple_element_t<I, meta_list>;
  static constexpr std::size_t kSize = std::tuple_size_v<meta_list>;

  template <MirrorMeta... Ms>
  explicit constexpr Mirror(ClassTag<ClassType>, Ms&&... ms) noexcept
      : meta_list_{CatMetaList(std::forward<Ms>(ms)...)} {}

  template <typename F>
  constexpr decltype(auto) Apply(F&& f) const noexcept {
    return std::apply(
        [&](auto const&... meta) -> decltype(auto) {
          return std::invoke(std::forward<F>(f), meta...);
        },
        meta_list_);
  }

  constexpr std::size_t size() const { return kSize; }

  template <std::size_t I>
    requires(I < kSize)
  constexpr decltype(auto) get() const {
    return std::get<I>(meta_list_);
  }

  meta_list meta_list_;
};

template <typename ClassType, MirrorMeta... Metas>
Mirror(ClassTag<ClassType>, Metas&&...)
    -> Mirror<ClassType, std::decay_t<Metas>...>;

template <typename T>
concept HasMirrorFunc = requires() {
  {
    std::decay_t<T>::template Mirror<std::decay_t<T>>()
  } noexcept -> MirrorType;
};

struct GetMirrorCpo {
  template <HasMirrorFunc T>
  constexpr auto operator()(T&&) const noexcept {
    using DecayedT = std::decay_t<T>;
    return DecayedT::template Mirror<DecayedT>();
  }
};

static inline constexpr auto get_mirror = GetMirrorCpo{};

}  // namespace ae::reflect

#endif  // AETHER_MISCPP_REFLECT_DETAILS_MIRROR_H_

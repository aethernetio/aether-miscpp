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

#ifndef AETHER_MISCPP_REFLECT_DETAILS_REFLECTION_H_
#define AETHER_MISCPP_REFLECT_DETAILS_REFLECTION_H_

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include "aether-miscpp/meta/as_type.h"
#include "aether-miscpp/reflect/details/meta.h"
#include "aether-miscpp/reflect/details/mirror.h"

namespace ae::reflect {
template <typename T>
struct IsMirror : std::false_type {};
template <typename ClassType, MirrorMeta... Metas>
struct IsMirror<Mirror<ClassType, Metas...>> : std::true_type {};
template <typename T>
inline constexpr auto IsMirror_v = IsMirror<T>::value;

/**
 * \brief Combine reference to obj and its Mirror.
 * Allow to get member names and values.
 */
template <typename C, typename Mirror>
  requires(IsMirror_v<Mirror> &&
           (std::is_same_v<std::decay_t<C>, typename Mirror::class_type> ||
            Mirror::kSize == 0))
class Reflection {
  template <typename U>
  static auto GetStoredClass(U&&) -> U;

 public:
  using class_type = C;
  using mirror_type = Mirror;
  // store by value if class_type is rvalue and store by reference otherwise
  using stored_class_type =
      std::conditional_t<std::is_void_v<class_type>, void,
                         decltype(GetStoredClass(std::declval<class_type>()))>;

  stored_class_type obj;
  mirror_type mirror;

  constexpr std::size_t size() const noexcept { return mirror.size(); }

  template <std::size_t I>
    requires(I < mirror_type::kSize)
  decltype(auto) get() noexcept {
    using meta_type = mirror_type::template meta_at<I>;
    return mirror.template get<I>().get(
        as_type<typename meta_type::class_type>(std::forward<class_type>(obj)));
  }

  template <std::size_t I>
    requires(I < mirror_type::kSize)
  decltype(auto) name() noexcept {
    return mirror.template get<I>().name;
  }

  template <typename F>
  decltype(auto) Apply(F&& func) noexcept {
    if constexpr (mirror_type::kSize == 0) {
      return std::invoke(std::forward<F>(func));
    } else {
      return mirror.Apply([&](auto const&... meta) -> decltype(auto) {
        return std::invoke(
            std::forward<F>(func),
            meta.get(as_type<typename std::decay_t<decltype(meta)>::class_type>(
                std::forward<class_type>(obj)))...);
      });
    }
  }
  template <typename F>
  decltype(auto) ApplyName(F&& func) noexcept {
    if constexpr (mirror_type::kSize == 0) {
      return std::invoke(std::forward<F>(func));
    }
    return mirror.Apply([&](auto const&... meta) -> decltype(auto) {
      return std::invoke(std::forward<F>(func), meta.name...);
    });
  }
  template <typename F>
  decltype(auto) ApplyMeta(F&& func) noexcept {
    if constexpr (mirror_type::kSize == 0) {
      return std::invoke(std::forward<F>(func));
    } else {
      return mirror.Apply([&](auto const&... meta) -> decltype(auto) {
        return std::invoke(std::forward<F>(func), std::forward<class_type>(obj),
                           meta...);
      });
    }
  }
};

// decltype(c) is used to get actual type of c i.e C&&, C&, C const& and not
// plain C
template <typename C, typename Mirror>
Reflection(C&& c, Mirror&& mirror)
    -> Reflection<decltype(c), std::decay_t<Mirror>>;

struct MakeReflection {
  template <typename C>
  constexpr auto operator()(C&& c) const noexcept {
    return Reflection(std::forward<C>(c), get_mirror(std::forward<C>(c)));
  }
};

static constexpr inline auto make_reflection = MakeReflection{};

template <typename C>
concept Reflectable = requires(C c) {
  { get_mirror(c) } -> MirrorType;
};

}  // namespace ae::reflect

#endif  // AETHER_MISCPP_REFLECT_DETAILS_REFLECTION_H_

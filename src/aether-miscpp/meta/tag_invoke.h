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

#ifndef AETHER_MISCPP_META_TAG_INVOKE_H_
#define AETHER_MISCPP_META_TAG_INVOKE_H_

#include <utility>

namespace ae {

namespace _tag_invoke {
void tag_invoke();

struct fn_ {
  template <typename Cpo, typename... Args>
  constexpr auto operator()(Cpo&& cpo, Args&&... args) const
      noexcept(noexcept(tag_invoke(std::forward<Cpo>(cpo),
                                   std::forward<Args>(args)...)))
          -> decltype(tag_invoke(std::forward<Cpo>(cpo),
                                 std::forward<Args>(args)...)) {
    return tag_invoke(std::forward<Cpo>(cpo), std::forward<Args>(args)...);
  }
};

template <typename Cpo, typename... Args>
using tag_invoke_result_t =
    decltype(tag_invoke(std::declval<Cpo&&>(), std::declval<Args&&>()...));

template <typename Cpo, typename... Args,
          typename = tag_invoke_result_t<Cpo, Args...>>
std::true_type TestTagInvocable(int) noexcept(
    noexcept(tag_invoke(std::declval<Cpo&&>(), std::declval<Args&&>()...)));

template <typename Cpo, typename... Args>
std::false_type TestTagInvocable(...);

}  // namespace _tag_invoke
inline constexpr auto tag_invoke = _tag_invoke::fn_{};

using _tag_invoke::tag_invoke_result_t;
using _tag_invoke::TestTagInvocable;

template <typename Cpo, typename... Args>
inline constexpr bool TagInvocable_v =
    decltype(TestTagInvocable<Cpo, Args...>(0))::value;

template <typename T, typename Cpo, typename... Args>
concept TagInvocable =
    TagInvocable_v<Cpo, T, Args...> || TagInvocable_v<Cpo, T&, Args...> ||
    TagInvocable_v<Cpo, T const&, Args...> || TagInvocable_v<Cpo, T&&, Args...>;

/**
 * \brief Helper struct to define cpo tag
 */
template <auto Cpo>
using tag_t = std::decay_t<decltype(Cpo)>;

}  // namespace ae

#endif  // AETHER_MISCPP_META_TAG_INVOKE_H_

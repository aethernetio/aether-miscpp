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

#ifndef AETHER_MISCPP_META_ARG_AT_H_
#define AETHER_MISCPP_META_ARG_AT_H_

#include <cstddef>
#include <utility>

namespace ae {
/**
 * \brief Get I'th argument in args list.
 */
template <std::size_t I, typename T, typename... TArgs>
constexpr decltype(auto) VarAt([[maybe_unused]] T&& arg,
                               [[maybe_unused]] TArgs&&... args) {
  static_assert(I <= sizeof...(args), "Index out of bounds");
  if constexpr (I == 0) {
    return std::forward<T>(arg);
  } else {
    return VarAt<I - 1>(std::forward<TArgs>(args)...);
  }
}

template <std::size_t I, auto Arg, auto... Args>
struct ArgAt {
  static_assert(I <= sizeof...(Args), "Index out of bounds");
  static constexpr auto value = ArgAt<I - 1, Args...>::value;
};

template <auto Arg, auto... Args>
struct ArgAt<0, Arg, Args...> {
  static constexpr auto value = Arg;
};

template <std::size_t I, auto... Args>
static inline constexpr auto ArgAt_v = ArgAt<I, Args...>::value;

}  // namespace ae

#endif  // AETHER_MISCPP_META_ARG_AT_H_

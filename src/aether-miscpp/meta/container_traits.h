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

#ifndef AETHER_MISCPP_META_CONTAINER_TRAITS_H_
#define AETHER_MISCPP_META_CONTAINER_TRAITS_H_

#include <string>
#include <type_traits>

namespace ae {
template <typename T>
struct IsString : std::false_type {};

template <>
struct IsString<std::string> : std::true_type {};

template <typename T>
struct IsStringView : std::false_type {};

template <>
struct IsStringView<std::string_view> : std::true_type {};

template <typename T, typename _ = void>
struct IsContainer : std::false_type {};

template <typename T>
struct IsContainer<
    T, std::conditional_t<
           false,
           std::void_t<typename T::value_type, typename T::size_type,
                       typename T::iterator, decltype(std::declval<T>().size()),
                       decltype(std::declval<T>().begin()),
                       decltype(std::declval<T>().end())>,
           void>> : public std::true_type {};

template <typename T, typename _ = void>
struct IsAssociatedContainer : std::false_type {};

template <typename T>
struct IsAssociatedContainer<
    T, std::conditional_t<
           false,
           std::void_t<typename T::key_type, typename T::mapped_type,
                       typename T::value_type, typename T::size_type,
                       typename T::iterator, typename T::const_iterator,
                       decltype(std::declval<T>().size()),
                       decltype(std::declval<T>().begin()),
                       decltype(std::declval<T>().end()),
                       decltype(std::declval<T>().cbegin()),
                       decltype(std::declval<T>().cend())>,
           void>> : public std::true_type {};

}  // namespace ae

#endif  // AETHER_MISCPP_META_CONTAINER_TRAITS_H_

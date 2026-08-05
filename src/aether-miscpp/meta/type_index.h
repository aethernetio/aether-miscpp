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

#ifndef AETHER_MISCPP_META_TYPE_INDEX_H_
#define AETHER_MISCPP_META_TYPE_INDEX_H_

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

#include "aether-miscpp/crc.h"

namespace ae {
template <typename T>
struct TypeNameHolder {
  template <std::size_t... Is>
  static constexpr auto NameArray(std::string_view str,
                                  std::index_sequence<Is...>) {
    return std::array{str[Is]...};
  }
  static constexpr auto GetTypeNameArray() {
#if defined __clang__
    constexpr std::string_view func_name = __PRETTY_FUNCTION__;
    constexpr std::string_view pre = "[T = ";
    constexpr std::string_view post = "]";
#elif defined __GNUC__
    constexpr std::string_view func_name = __PRETTY_FUNCTION__;
    constexpr std::string_view pre = "[with T = ";
    constexpr std::string_view post = "]";
#elif defined _MSC_VER
    constexpr std::string_view func_name = __FUNCSIG__;
    constexpr std::string_view pre = "TypeNameHolder<";
    constexpr std::string_view post = ">::GetTypeNameArray(void)";
#else
    constexpr std::string_view func_name = "unsupported";
    constexpr std::string_view pre = "";
    constexpr std::string_view post = "";
#endif
    constexpr auto begin = func_name.find(pre) + pre.size();
    constexpr auto end = func_name.rfind(post);
    static_assert(begin < end);
    return NameArray(func_name.substr(begin, end),
                     std::make_index_sequence<end - begin>());
  }
  static constexpr auto kNameArray = GetTypeNameArray();
};

template <typename T>
constexpr auto GetTypeName() {
  constexpr auto& value = TypeNameHolder<T>::kNameArray;
  return std::string_view{value.data(), value.size()};
}

template <typename T>
constexpr auto GetTypeIndex() {
  return crc32::from_string_view(GetTypeName<T>()).value;
}
}  // namespace ae

#endif  // AETHER_MISCPP_META_TYPE_INDEX_H_

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

#ifndef AETHER_MISCPP_FORMAT_FORMATTER_H_
#define AETHER_MISCPP_FORMAT_FORMATTER_H_

#include <string_view>

namespace ae {
// Context passed to Formatter<T>::Format.  Custom formatters should write via
// ctx.out().write(...).  ctx.options contains text from placeholders written as
// {:...}; placeholders without ':' provide an empty options string.
template <typename Writer>
class FormatContext {
 public:
  constexpr FormatContext(Writer& out, std::string_view opt_string)
      : options{opt_string}, out_{&out} {}

  constexpr auto& out() { return *out_; }

  std::string_view options;

 private:
  Writer* out_;
};

// To format a custom type, specialize ae::Formatter<MyType> and implement:
//   template <typename Writer>
//   void Format(MyType const&, ae::FormatContext<Writer>& ctx) const;
template <typename T, typename Enable = void>
struct Formatter {};

}  // namespace ae

#endif  // AETHER_MISCPP_FORMAT_FORMATTER_H_

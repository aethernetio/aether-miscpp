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

#ifndef AETHER_MISCPP_FORMAT_FORMAT_IMPL_H_
#define AETHER_MISCPP_FORMAT_FORMAT_IMPL_H_

#include <array>
#include <concepts>
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "aether-miscpp/format/formatter.h"

namespace ae {
namespace format_internal {

struct StringWriter {
  explicit StringWriter(std::string& out) noexcept : out_{&out} {}
  void write(std::string_view str) { out_->append(str.data(), str.size()); }
  void write(char ch) { out_->push_back(ch); }
  std::string* out_;
};

struct OStreamWriter {
  explicit OStreamWriter(std::ostream& out) noexcept : out_{&out} {}
  void write(std::string_view str) {
    out_->write(str.data(), static_cast<std::streamsize>(str.size()));
  }
  void write(char ch) { out_->put(ch); }
  std::ostream* out_;
};

struct FormatPart {
  std::size_t offset{};
  std::size_t size{};
  bool placeholder{};
};

struct FormatScheme {
  // Hard limit for parsed parts, including both placeholders and literal runs.
  // The current layout supports the common maximum pattern of 10 placeholders
  // interleaved with 11 literals. If parsing would exceed this 21-part limit,
  // overflow is recorded; formatting then writes the original source followed
  // by " OVERFLOW" and does not format any arguments.
  static constexpr std::size_t kMaxFormatParts = 21;

  template <std::size_t N>
  // NOLINTNEXTLINE(*explicit-constructor*)
  constexpr FormatScheme(char const (&format)[N]) noexcept
      : FormatScheme{std::string_view{format, N ? N - 1 : 0}} {}
  // NOLINTNEXTLINE(*explicit-constructor*)
  constexpr FormatScheme(std::string_view format) noexcept : source{format} {
    Parse();
  }

  constexpr std::size_t reserve_hint() const noexcept { return source.size(); }

  // Non-owning format source. Referenced characters must remain alive and
  // unchanged while formatting.
  std::string_view source;
  std::array<FormatPart, kMaxFormatParts> parts{};
  std::size_t part_count{};
  bool overflow{};

 private:
  constexpr void AddPart(std::size_t offset, std::size_t size,
                         bool placeholder) noexcept {
    if (size == 0) {
      return;
    }
    if (part_count == parts.size()) {
      overflow = true;
      return;
    }
    parts[part_count++] = FormatPart{offset, size, placeholder};
  }

  constexpr void Parse() noexcept {
    std::size_t literal_begin = 0;
    std::size_t i = 0;
    while (i < source.size()) {
      auto const escaped_brace = i + 1 < source.size() &&
                                 (source[i] == '{' || source[i] == '}') &&
                                 source[i + 1] == source[i];
      if (escaped_brace) {
        AddPart(literal_begin, i - literal_begin, false);
        AddPart(i, 1, false);
        i += 2;
        literal_begin = i;
        continue;
      }

      if (source[i] == '{') {
        auto close = i + 1;
        while (close < source.size() && source[close] != '}') {
          ++close;
        }
        if (close == source.size()) {
          break;
        }
        AddPart(literal_begin, i - literal_begin, false);
        AddPart(i, close - i + 1, true);
        i = close + 1;
        literal_begin = i;
      } else {
        ++i;
      }
    }
    AddPart(literal_begin, source.size() - literal_begin, false);
  }
};

template <typename T, typename Writer>
concept HasFormatterFor =
    requires(Formatter<std::decay_t<T>> formatter, T const& value,
             FormatContext<Writer>& ctx) { formatter.Format(value, ctx); };

template <typename Writer>
concept WriterLike = requires(Writer& writer) {
  writer.write(std::string_view{});
  writer.write('a');
};

template <typename Writer, typename T>
void FormatValue(Writer& writer, T const& value, std::string_view options) {
  using U = std::decay_t<T>;
  if constexpr (HasFormatterFor<U, Writer>) {
    auto ctx = FormatContext<Writer>{writer, options};
    Formatter<U>{}.Format(value, ctx);
  } else {
    static_assert(sizeof(U) == 0, "Unsupported type for ae::Format");
  }
}

template <typename Writer>
struct FormatArg {
  using FormatFn = void (*)(Writer&, void const*, std::string_view);
  void const* value{};
  FormatFn format{};
};

template <typename Writer, typename T>
void FormatArgThunk(Writer& writer, void const* value,
                    std::string_view options) {
  FormatValue(writer, *static_cast<T const*>(value), options);
}

template <typename Writer, typename T>
FormatArg<Writer> MakeFormatArg(T const& value) {
  return FormatArg<Writer>{
      static_cast<void const*>(std::addressof(value)),
      &FormatArgThunk<Writer, T>,
  };
}

constexpr std::string_view PlaceholderOptions(
    std::string_view placeholder) noexcept {
  if ((placeholder.size() >= 3) && (placeholder[0] == '{') &&
      (placeholder[1] == ':') && (placeholder.back() == '}')) {
    return std::string_view{placeholder.data() + 2, placeholder.size() - 3};
  }
  return {};
}

template <typename Writer>
void FormatToWriterErased(Writer& writer, FormatScheme const& scheme,
                          FormatArg<Writer> const* args,
                          std::size_t arg_count) {
  if (scheme.overflow) {
    writer.write(scheme.source);
    writer.write(" OVERFLOW");
    return;
  }

  std::size_t next_arg = 0;
  for (std::size_t i = 0; i < scheme.part_count; ++i) {
    auto const part = scheme.parts[i];
    auto const text = scheme.source.substr(part.offset, part.size);
    if (part.placeholder) {
      auto const arg_index = next_arg;
      auto const options = PlaceholderOptions(text);
      if (next_arg < arg_count) {
        ++next_arg;
        args[arg_index].format(writer, args[arg_index].value, options);
      } else {
        writer.write(text);
      }
    } else {
      writer.write(text);
    }
  }
}

template <typename Writer, typename... Args>
void FormatToWriter(Writer& writer, FormatScheme const& scheme,
                    Args&&... args) {
  auto erased_args = std::array<FormatArg<Writer>, sizeof...(Args)>{
      MakeFormatArg<Writer>(args)...,
  };
  FormatToWriterErased(writer, scheme, erased_args.data(), erased_args.size());
}

}  // namespace format_internal

using FormatScheme = format_internal::FormatScheme;

template <typename TStream, typename... Args>
  requires std::derived_from<std::remove_reference_t<TStream>, std::ostream>
void FormatTo(TStream& stream, FormatScheme const& format, Args&&... args) {
  auto writer = format_internal::OStreamWriter{stream};
  format_internal::FormatToWriter(writer, format, std::forward<Args>(args)...);
}

template <format_internal::WriterLike Writer, typename... Args>
void FormatTo(Writer& writer, FormatScheme const& format, Args&&... args) {
  format_internal::FormatToWriter(writer, format, std::forward<Args>(args)...);
}

template <typename... Args>
void FormatTo(std::string& out, FormatScheme const& format, Args&&... args) {
  // Precondition: format.source and formatted arguments must not reference
  // storage owned by out. This includes string_view, char const*, containers,
  // ranges, strings, or views into out. This overload may reserve/append to out
  // before or while reading inputs, which can invalidate or overlap those
  // reads. No runtime aliasing checks are performed.
  out.reserve(out.size() + format.reserve_hint());
  auto writer = format_internal::StringWriter{out};
  format_internal::FormatToWriter(writer, format, std::forward<Args>(args)...);
}

template <typename TStream, typename... Args>
  requires std::derived_from<std::remove_reference_t<TStream>, std::ostream>
void Format(TStream& stream, FormatScheme const& format, Args&&... args) {
  FormatTo(stream, format, std::forward<Args>(args)...);
}

template <typename... Args>
std::string Format(FormatScheme const& format, Args&&... args) {
  std::string out;
  FormatTo(out, format, std::forward<Args>(args)...);
  return out;
}

}  // namespace ae

#endif  // AETHER_MISCPP_FORMAT_FORMAT_IMPL_H_

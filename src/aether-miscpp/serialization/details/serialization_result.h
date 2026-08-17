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

#ifndef AETHER_SERIALIZATION_DETAILS_SERIALIZATION_RESULT_H_
#define AETHER_SERIALIZATION_DETAILS_SERIALIZATION_RESULT_H_

#include <cassert>
#include <functional>
#include <string_view>
#include <utility>

#include "aether-miscpp/types/result.h"

namespace ae::seri {
struct SeriError {
  int error_code;
  std::string_view message;
};

inline constexpr SeriError read_error{.error_code = 1, .message = "Read error"};
inline constexpr SeriError read_eof{.error_code = 2,
                                    .message = "Read buffer is EOF"};
inline constexpr SeriError write_error{.error_code = 3,
                                       .message = "Write error"};
inline constexpr SeriError write_eof{.error_code = 4,
                                     .message = "Write buffer is EOF"};
inline constexpr SeriError container_too_large{
    .error_code = 5, .message = "Container too large"};
inline constexpr SeriError invalid_bool{.error_code = 6,
                                        .message = "Invalid bool value"};
inline constexpr SeriError invalid_variant_index{
    .error_code = 7, .message = "Invalid variant index"};

struct Good {};

inline constexpr Good good{};
}  // namespace ae::seri

namespace ae {
#ifdef _MSC_VER
#  pragma warning(push)
// warning C4702: unreachable code
// this looks like false-positive on MSVCx86, so suppress it
#  pragma warning(disable : 4702)
#endif

// minimal specialiazation for SeriResult
template <>
class Result<seri::Good, seri::SeriError> {
 public:
  using value_type = seri::Good;
  using error_type = seri::SeriError;

  // NOLINTNEXTLINE(*explicit*,*rvalue*)
  constexpr Result(Ok<seri::Good>&& ok_val) noexcept : ok_{true} {
    // explicitly discard value
    (void)ok_val;
  }
  // NOLINTNEXTLINE(*explicit*)
  constexpr Result(Error<seri::SeriError>&& err) noexcept
      : ok_{false}, error_{std::move(err).error} {}

  // since SeriError is trivially destructible Result destructor is default
  static_assert(std::is_trivially_destructible_v<seri::SeriError>,
                "Keep seri error trivially destructible");

  constexpr ~Result() noexcept = default;

  // NOLINTNEXTLINE(*static*)
  constexpr seri::Good value() const noexcept {
    assert(ok_);  // NOLINT(*static-assert*)
    return seri::good;
  }
  constexpr seri::SeriError const& error() const noexcept {
    assert(!ok_);  // NOLINT(*static-assert*)
    return error_;
  }
  constexpr seri::SeriError& error() noexcept {
    assert(!ok_);  // NOLINT(*static-assert*)
    return error_;
  }

  constexpr bool IsOk() const noexcept { return ok_; }
  constexpr bool IsErr() const noexcept { return !ok_; }
  constexpr explicit operator bool() const noexcept { return ok_; }

  template <typename F, typename R = std::invoke_result_t<F>>
    requires(requires {
      // F must return result type
      { IsResultType_v<R> };
      // result must be with same error_type
      { ResultType<R, typename R::value_type, error_type> };
    })
  auto Then(F&& f) && -> std::invoke_result_t<F> {
    if (IsOk()) {
      return std::invoke(std::forward<F>(f));
    }
    return Error{std::move(*this).error()};
  }

  template <typename FE, typename R = std::invoke_result_t<FE, error_type&&>>
    requires(requires {
      // FE must return result type
      { IsResultType_v<R> };
      // result must be with same value_type
      { ResultType<R, value_type, typename R::error_type> };
    })
  auto Else(FE&& f) && -> std::invoke_result_t<FE, error_type&&> {
    if (IsErr()) {
      return std::invoke(std::forward<FE>(f), std::move(*this).error());
    }
    return Ok{std::move(*this).value()};
  }

  template <typename FE, typename R = std::invoke_result_t<FE>>
    requires(requires {
      // FE must return result type
      { IsResultType_v<R> };
      // result must be with same value_type
      { ResultType<R, value_type, typename R::error_type> };
    })
  auto Else(FE&& f) && -> std::invoke_result_t<FE> {
    if (IsErr()) {
      return std::invoke(std::forward<FE>(f));
    }
    return Ok{std::move(*this).value()};
  }

 private:
  bool ok_;
  // error_ is under union to not initiate in by default
  union {
    seri::SeriError error_;
  };
};

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

}  // namespace ae

namespace ae::seri {
using SeriResult = Result<Good, SeriError>;
}  // namespace ae::seri

#endif  // AETHER_SERIALIZATION_DETAILS_SERIALIZATION_RESULT_H_

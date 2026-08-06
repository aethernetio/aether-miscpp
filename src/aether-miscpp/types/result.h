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

#ifndef AETHER_MISCPP_TYPES_RESULT_H_
#define AETHER_MISCPP_TYPES_RESULT_H_

#include <cassert>
#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace ae {
template <typename T, typename E>
class Result;

template <typename T>
struct IsResultType : std::false_type {};
template <typename V, typename E>
struct IsResultType<Result<V, E>> : std::true_type {};
template <typename T>
static inline constexpr bool IsResultType_v = IsResultType<T>::value;

template <typename T, typename V, typename E>
concept ResultType = requires(T t) {
  { IsResultType_v<T> };
  { std::same_as<V, typename T::value_type> };
  { std::same_as<E, typename T::error_type> };
};

template <typename T>
struct Ok {
  explicit Ok(T v) : value{std::move(v)} {}
  [[no_unique_address]] T value;
};

template <typename T>
struct Ok<T&> {
  explicit Ok(T& v) : value{v} {}
  [[no_unique_address]] T& value;
};

template <typename E>
struct Error {
  explicit Error(E v) : error{std::move(v)} {}
  [[no_unique_address]] E error;
};

template <typename E>
struct Error<E&> {
  explicit Error(E& v) : error{v} {}
  [[no_unique_address]] E& error;
};

template <typename T, typename E>
class Result {
  template <typename U>
  struct OkVal {
    [[no_unique_address]] U v;
  };
  template <typename U>
  struct ErrVal {
    [[no_unique_address]] U v;
  };

 public:
  using value_type = T;
  using error_type = E;

  using store_value_type = OkVal<std::conditional_t<
      std::is_reference_v<T>,
      std::reference_wrapper<std::remove_reference_t<T>>, T>>;
  using store_error_type = ErrVal<std::conditional_t<
      std::is_reference_v<E>,
      std::reference_wrapper<std::remove_reference_t<E>>, E>>;

  template <typename U>
    requires(std::is_same_v<T, U>)
  explicit Result(U&& value)
      : storage_{store_value_type{std::forward<U>(value)}} {}

  template <typename EU>
    requires(std::is_same_v<E, EU>)
  explicit Result(EU&& error)
      : storage_{store_error_type{std::forward<EU>(error)}} {}

  // made implicit intentionally
  constexpr Result(Ok<T>&& ok)  // NOLINT(*explicit-constructor)
      : storage_{store_value_type{std::move(ok).value}} {}
  // made implicit intentionally
  constexpr Result(Error<E>&& error)  // NOLINT(*explicit-constructor)
      : storage_{store_error_type{std::move(error).error}} {}

  constexpr bool IsOk() const noexcept { return storage_.index() == 0; }
  constexpr bool IsErr() const noexcept { return storage_.index() == 1; }
  constexpr explicit operator bool() const noexcept {
    return storage_.index() == 0;
  }

  auto& value() & noexcept {
    assert(IsOk());
    return static_cast<value_type&>(get_value()->v);
  }
  auto const& value() const& noexcept {
    assert(IsOk());
    return static_cast<value_type const&>(get_value()->v);
  }
  auto&& value() && noexcept {
    assert(IsOk());
    return std::move(static_cast<value_type&>(get_value()->v));
  }

  auto& error() & noexcept {
    assert(IsErr());
    return static_cast<error_type&>(get_error()->v);
  }
  auto const& error() const& noexcept {
    assert(IsErr());
    return static_cast<error_type const&>(get_error()->v);
  }
  auto&& error() && noexcept {
    assert(IsErr());
    return std::move(static_cast<error_type&>(get_error()->v));
  }

  template <typename F, typename R = std::invoke_result_t<F, value_type&&>>
    requires(requires {
      // F must return result type
      { IsResultType_v<R> };
      // result must be with same error_type
      { ResultType<R, typename R::value_type, error_type> };
    })
  auto Then(F&& f) && -> std::invoke_result_t<F, value_type&&> {
    if (IsOk()) {
      return std::invoke(std::forward<F>(f), std::move(*this).value());
    }
    return Error{std::move(*this).error()};
  }

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
  // dereference and take address to get value from reference wrapper
  store_value_type* get_value() & { return std::get_if<0>(&storage_); }
  store_value_type const* get_value() const& {
    return std::get_if<0>(&storage_);
  }
  store_error_type* get_error() & { return std::get_if<1>(&storage_); }
  store_error_type const* get_error() const& {
    return std::get_if<1>(&storage_);
  }

  std::variant<store_value_type, store_error_type> storage_;
};
}  // namespace ae

#define TRY_VALUE(VAR_NAME, ...)                            \
  auto _RES_##VAR_NAME = __VA_ARGS__;                       \
  if (_RES_##VAR_NAME.IsErr()) {                            \
    return ::ae::Error{std::move(_RES_##VAR_NAME).error()}; \
  }                                                         \
  auto VAR_NAME = _RES_##VAR_NAME.value()

#define TRY_RESULT(...)                                             \
  if (auto _RES_ = __VA_ARGS__; _RES_.IsErr()) return ::ae::Error { \
      std::move(_RES_).error()                                      \
    }

#endif  // AETHER_MISCPP_TYPES_RESULT_H_

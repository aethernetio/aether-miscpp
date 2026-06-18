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

#ifndef AETHER_MISCPP_MISC_DEFER_H_
#define AETHER_MISCPP_MISC_DEFER_H_

#include <utility>
#include <optional>
#include <type_traits>

namespace ae {
/**
 * \brief Defers the call of callable to the end of scope
 */
template <typename T>
  requires std::is_invocable_v<T>
class ScopeExit {
 public:
  constexpr explicit ScopeExit(T const& cb) noexcept(
      std::is_nothrow_copy_constructible_v<T>)
      : callable_{cb} {}
  constexpr explicit ScopeExit(T&& cb) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : callable_{std::move(cb)} {}

  ~ScopeExit() noexcept(std::is_nothrow_invocable_v<T>) {
    if (!callable_) {
      return;
    }
    (*callable_)();
  }

  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ScopeExit(ScopeExit&&) = delete;
  ScopeExit& operator=(ScopeExit&&) = delete;

  void Reset() noexcept { callable_.reset(); }

 private:
  std::optional<T> callable_;
};

template <typename T>
ScopeExit(T&&) -> ScopeExit<T>;

struct MakeScopeExit {};

template <typename TCallable>
constexpr auto operator<<(MakeScopeExit, TCallable&& cb) {
  return ScopeExit(std::forward<TCallable>(cb));
}
}  // namespace ae

#define ae_defer_at MakeScopeExit{} <<
#define AEMSCPP_CONCAT_(a, b) a##b
#define AEMSCPP_CONCAT(a, b) AEMSCPP_CONCAT_(a, b)
#define ae_defer auto AEMSCPP_CONCAT(SCOPE_GUARD_, __LINE__) = ae_defer_at

#endif  // AETHER_MISCPP_MISC_DEFER_H_

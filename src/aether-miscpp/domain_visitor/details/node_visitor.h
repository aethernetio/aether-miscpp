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
#ifndef AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_NODE_VISITOR_H_
#define AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_NODE_VISITOR_H_
#include <functional>
#include <type_traits>
#include <utility>

#include "aether-miscpp/domain_visitor/details/cycle_detector.h"

namespace ae::domain_visitor {

// NOLINTNEXTLINE(*use-enum-class)
enum VisitPolicy {
  kNone = 0,
  kShallow = 1,
  kReflection = 1 << 1,
  kDeep = 1 << 2,
  kPointers = 1 << 3,
  kAny = 0xfffffff
};

template <VisitPolicy policy>
struct PolicyMatch {
  static constexpr bool Match(int val) { return (val & policy) > 0; }
};

template <>
struct PolicyMatch<VisitPolicy::kAny> {
  static constexpr bool Match(int) { return true; }
};

using AnyPolicyMatch = PolicyMatch<VisitPolicy::kAny>;

template <int Policy>
struct PolicyConst {
  static constexpr int kPolicy = Policy;
};

template <typename T, typename Enable = void>
struct NodeVisitor;

template <typename Visitor, int policy>
struct DomainNodeVisitor;

template <typename U, typename Visitor>
bool CallVisitor(U&& obj, Visitor&& visitor) {
  if constexpr (std::is_invocable_v<Visitor, U>) {
    if constexpr (std::is_same_v<bool, std::invoke_result_t<Visitor, U>>) {
      return std::invoke(std::forward<Visitor>(visitor), std::forward<U>(obj));
    }
    std::invoke(std::forward<Visitor>(visitor), std::forward<U>(obj));
    return true;
  } else {
    return true;
  }
}

template <typename T, typename Visitor, int policy>
void DomainVisit(CycleDetector&, T&&,
                 DomainNodeVisitor<Visitor, policy> const&);

template <typename U, typename Visitor>
void ApplyVisitorShallow(U&& obj, CycleDetector&, Visitor&& visitor) {
  CallVisitor(std::forward<U>(obj), std::forward<Visitor>(visitor));
}

template <typename U, typename Visitor>
// NOLINTNEXTLINE(misc-no-recursion): Domain traversal intentionally recurses.
void ApplyVisitorRest(U&& obj, CycleDetector& cd, Visitor&& visitor) {
  if (CallVisitor(std::forward<U>(obj), visitor)) {
    DomainVisit(cd, std::forward<U>(obj), std::forward<Visitor>(visitor));
  }
}

template <typename U, typename Visitor>
void ApplyVisitor(U&& obj, CycleDetector& cd, Visitor&& visitor) {
  using VisitorType = std::remove_cvref_t<Visitor>;
  // Apply shallow behavior only for the exact shallow policy.
  if constexpr (VisitorType::kPolicy == VisitPolicy::kShallow) {
    ApplyVisitorShallow(std::forward<U>(obj), cd,
                        std::forward<Visitor>(visitor));
  } else {
    ApplyVisitorRest(std::forward<U>(obj), cd, std::forward<Visitor>(visitor));
  }
}

template <typename T, typename Enable = void>
struct HasNodeVisitor : std::false_type {};

template <typename T>
struct HasNodeVisitor<T, std::void_t<decltype(NodeVisitor<T>{})>>
    : std::true_type {};
}  // namespace ae::domain_visitor
#endif

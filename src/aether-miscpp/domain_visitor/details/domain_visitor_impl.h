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
#ifndef AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_DOMAIN_VISITOR_IMPL_H_
#define AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_DOMAIN_VISITOR_IMPL_H_
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "aether-miscpp/domain_visitor/details/node_visitor.h"

namespace ae::domain_visitor {

template <typename Visitor, int policy>
struct DomainNodeVisitor {
  static constexpr auto kPolicy = policy;

  explicit constexpr DomainNodeVisitor(Visitor vis)
      : visitor{std::forward<Visitor>(vis)} {}

  template <typename U>
    requires(std::is_invocable_v<Visitor const&, U>)
  decltype(auto) operator()(U&& val) const {
    return std::invoke(std::as_const(visitor), std::forward<U>(val));
  }

  Visitor visitor;
};

template <typename Visitor, int policy>
struct DomainNodeVisitor<Visitor&, policy> {
  static constexpr auto kPolicy = policy;

  explicit constexpr DomainNodeVisitor(Visitor& vis) : visitor{vis} {}

  template <typename U>
    requires(std::is_invocable_v<Visitor&, U>)
  decltype(auto) operator()(U&& val) const {
    return std::invoke(visitor, std::forward<U>(val));
  }

  Visitor& visitor;
};

template <typename Visitor, int policy>
struct DomainNodeVisitor<Visitor&&, policy> {
  static constexpr auto kPolicy = policy;

  explicit constexpr DomainNodeVisitor(Visitor&& vis)
      : visitor{std::forward<Visitor>(vis)} {}

  template <typename U>
    requires(std::is_invocable_v<Visitor const&, U>)
  decltype(auto) operator()(U&& val) const {
    return std::invoke(std::as_const(visitor), std::forward<U>(val));
  }

  Visitor&& visitor;
};

template <typename T>
struct IsDomainNodeVisitor : std::false_type {};

template <typename V, int policy>
struct IsDomainNodeVisitor<DomainNodeVisitor<V, policy>> : std::true_type {};

template <typename T>
inline constexpr bool IsDomainNodeVisitor_v =
    IsDomainNodeVisitor<std::remove_cvref_t<T>>::value;

template <typename T, typename Visitor, int policy = VisitPolicy::kShallow>
  requires(!IsDomainNodeVisitor_v<Visitor>)
void DomainVisit(T&& obj, Visitor&& visitor, PolicyConst<policy> = {}) {
  auto cycle_detector = CycleDetector{};
  auto domain_visitor =
      DomainNodeVisitor<Visitor, policy>{std::forward<Visitor>(visitor)};
  DomainVisit(cycle_detector, std::forward<T>(obj), domain_visitor);
}

template <typename T, typename Visitor, int policy = VisitPolicy::kShallow>
void DomainVisit(T&& obj, DomainNodeVisitor<Visitor, policy> const& visitor) {
  auto cycle_detector = CycleDetector{};
  DomainVisit(cycle_detector, std::forward<T>(obj), visitor);
}

template <typename T, typename Visitor, int policy = VisitPolicy::kShallow>
// NOLINTNEXTLINE(misc-no-recursion): Domain traversal intentionally recurses.
void DomainVisit(CycleDetector& cd, T&& obj,
                 DomainNodeVisitor<Visitor, policy> const& visitor) {
  if constexpr (HasNodeVisitor<std::decay_t<T>>::value) {
    using Nv = NodeVisitor<std::decay_t<T>>;
    if constexpr (policy == VisitPolicy::kShallow ||
                  Nv::Policy::Match(policy)) {
      if constexpr (!std::is_pointer_v<std::remove_reference_t<T>>) {
        if (!cd.Add(std::addressof(obj))) {
          return;
        }
      }
      auto node_visitor = Nv{};
      std::as_const(node_visitor).Visit(std::forward<T>(obj), cd, visitor);
    }
  }
}
}  // namespace ae::domain_visitor
#endif

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
#ifndef AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_PTR_LIKE_NODE_VISITOR_H_
#define AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_PTR_LIKE_NODE_VISITOR_H_

#include "aether-miscpp/domain_visitor/details/node_visitor.h"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace ae::domain_visitor {

template <typename T>
  requires(HasNodeVisitor<std::remove_const_t<T>>::value)
struct NodeVisitor<T*, void> {
  using Policy = PolicyMatch<VisitPolicy::kPointers>;

  template <typename Visitor>
  void Visit(T* obj, CycleDetector& cd, Visitor&& visitor) const {
    if (obj != nullptr) {
      DomainVisit(cd, *obj, std::forward<Visitor>(visitor));
    }
  }
};

template <typename T>
  requires(HasNodeVisitor<std::remove_const_t<T>>::value)
struct NodeVisitor<std::unique_ptr<T>, void> {
  using Policy = PolicyMatch<VisitPolicy::kPointers>;

  template <typename Visitor>
  void Visit(std::unique_ptr<T> const& obj, CycleDetector& cd,
             Visitor&& visitor) const {
    if (obj) {
      DomainVisit(cd, *obj, std::forward<Visitor>(visitor));
    }
  }
};

template <typename T>
  requires(HasNodeVisitor<std::remove_const_t<T>>::value)
struct NodeVisitor<std::shared_ptr<T>, void> {
  using Policy = PolicyMatch<VisitPolicy::kPointers>;

  template <typename Visitor>
  void Visit(std::shared_ptr<T> const& obj, CycleDetector& cd,
             Visitor&& visitor) const {
    if (obj) {
      DomainVisit(cd, *obj, std::forward<Visitor>(visitor));
    }
  }
};

template <typename T>
  requires(HasNodeVisitor<std::remove_const_t<T>>::value)
struct NodeVisitor<std::reference_wrapper<T>, void> {
  using Policy = PolicyMatch<VisitPolicy::kPointers>;

  template <typename Visitor>
  void Visit(std::reference_wrapper<T>& obj, CycleDetector& cd,
             Visitor&& visitor) const {
    DomainVisit(cd, static_cast<T&>(obj), std::forward<Visitor>(visitor));
  }

  template <typename Visitor>
  void Visit(std::reference_wrapper<T> const& obj, CycleDetector& cd,
             Visitor&& visitor) const {
    DomainVisit(cd, static_cast<T&>(obj), std::forward<Visitor>(visitor));
  }
};

}  // namespace ae::domain_visitor
#endif

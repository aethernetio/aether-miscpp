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

#ifndef AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_CONTAINER_NODE_VISITOR_H_
#define AETHER_MISCPP_DOMAIN_VISITOR_DETAILS_CONTAINER_NODE_VISITOR_H_

#include <utility>

#include "aether-miscpp/domain_visitor/details/node_visitor.h"
#include "aether-miscpp/meta/container_traits.h"

namespace ae::domain_visitor {

template <typename T>
  requires(IsContainer<T>::value)
struct NodeVisitor<T, void> {
  using Policy = PolicyMatch<VisitPolicy::kDeep>;

  template <typename Visitor>
  void Visit(T& obj, CycleDetector& cd, Visitor&& visitor) const {
    auto&& vtr = std::forward<Visitor>(visitor);
    for (auto& v : obj) {
      ApplyVisit(v, cd, vtr);
    }
  }

  template <typename Visitor>
  void Visit(T const& obj, CycleDetector& cd, Visitor&& visitor) const {
    auto&& vtr = std::forward<Visitor>(visitor);
    for (auto& v : obj) {
      ApplyVisit(v, cd, vtr);
    }
  }

  template <typename U, typename Visitor>
  void ApplyVisit(U&& obj, CycleDetector& cd, Visitor&& visitor) const {
    ApplyVisitor(std::forward<U>(obj), cd, std::forward<Visitor>(visitor));
  }
};

template <typename K, typename V>
struct NodeVisitor<std::pair<K, V>> {
  using Policy = PolicyMatch<VisitPolicy::kDeep>;

  template <typename Visitor>
  void Visit(std::pair<K, V>& obj, CycleDetector& cd, Visitor&& visitor) const {
    ApplyVisit(obj.second, cd, std::forward<Visitor>(visitor));
  }

  template <typename Visitor>
  void Visit(std::pair<K, V> const& obj, CycleDetector& cd,
             Visitor&& visitor) const {
    ApplyVisit(obj.second, cd, std::forward<Visitor>(visitor));
  }

  template <typename U, typename Visitor>
  void ApplyVisit(U&& obj, CycleDetector& cd, Visitor&& visitor) const {
    ApplyVisitor(std::forward<U>(obj), cd, std::forward<Visitor>(visitor));
  }
};

}  // namespace ae::domain_visitor

#endif

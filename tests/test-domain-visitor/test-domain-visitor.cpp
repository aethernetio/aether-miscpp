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
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <unity.h>

#include "aether-miscpp/domain_visitor/details/cycle_detector.h"
#include "aether-miscpp/domain_visitor/details/domain_visitor_impl.h"
#include "aether-miscpp/domain_visitor/details/node_visitor.h"
#include "aether-miscpp/domain_visitor/domain_visitor.h"
#include "aether-miscpp/misc/override.h"
#include "aether-miscpp/reflect/reflect.h"

namespace test_domain_visitor {
constexpr int kExpectedVisitedValue = 42;
constexpr int kExpectedDeepVisitedValue = 7;

struct Node {
  int value{};
  Node* next{};

  AE_REFLECT_MEMBERS(value, next)
};

static_assert(ae::domain_visitor::HasNodeVisitor<Node>::value);
static_assert(!ae::domain_visitor::HasNodeVisitor<Node const>::value);
static_assert(ae::domain_visitor::HasNodeVisitor<Node const*>::value);
static_assert(
    ae::domain_visitor::HasNodeVisitor<std::unique_ptr<Node const>>::value);
static_assert(
    ae::domain_visitor::HasNodeVisitor<std::shared_ptr<Node const>>::value);
static_assert(ae::domain_visitor::HasNodeVisitor<
              std::reference_wrapper<Node const>>::value);

struct Wrapper {
  Node node{};

  AE_REFLECT_MEMBERS(node)
};

struct Leaf {
  int value{};

  AE_REFLECT_MEMBERS(value)
};

struct Root {
  Leaf leaf{};

  AE_REFLECT_MEMBERS(leaf)
};

struct SmartPointerRoot {
  std::unique_ptr<Node> unique_node;
  std::shared_ptr<Node> shared_node;

  AE_REFLECT_MEMBERS(unique_node, shared_node)
};

struct AliasingRoot {
  std::shared_ptr<Node> first;
  std::shared_ptr<Node> second;

  AE_REFLECT_MEMBERS(first, second)
};

struct SmartNode {
  int value{};
  std::shared_ptr<SmartNode> next;

  AE_REFLECT_MEMBERS(value, next)
};

struct VisitBase {
  int inherited{};

  AE_REFLECT_MEMBERS(inherited)
};

struct VisitDerived : VisitBase {
  AE_REFLECT(AE_BASE(VisitBase))
};

struct MutableIntCallback {
  mutable int visits{};

  void operator()(int) const { ++visits; }
};

struct NonCopyableIntCallback {
  explicit NonCopyableIntCallback(int& visit_count)
      : visit_count{visit_count} {}

  NonCopyableIntCallback(NonCopyableIntCallback const&) = delete;
  NonCopyableIntCallback& operator=(NonCopyableIntCallback const&) = delete;

  void operator()(int) { ++visit_count; }

  int& visit_count;
};

struct MoveOnlyIntCallback {
  explicit MoveOnlyIntCallback(int& visit_count)
      : visit_count{std::make_unique<int*>(std::addressof(visit_count))} {}

  MoveOnlyIntCallback(MoveOnlyIntCallback const&) = delete;
  MoveOnlyIntCallback& operator=(MoveOnlyIntCallback const&) = delete;
  MoveOnlyIntCallback(MoveOnlyIntCallback&&) = default;
  MoveOnlyIntCallback& operator=(MoveOnlyIntCallback&&) = default;

  void operator()(int) const { ++**visit_count; }

  std::unique_ptr<int*> visit_count;
};

struct InvocationCategoryCallback {
  mutable int const_lvalue_visits{};
  int rvalue_visits{};

  void operator()(int) const& { ++const_lvalue_visits; }
  void operator()(int) && { ++rvalue_visits; }
};
}  // namespace test_domain_visitor

namespace {

using namespace test_domain_visitor;

// Verifies reference fallback terminates traversal of raw-pointer cycles.
void test_CycleWithReferenceFallback() {
  Node a{1, nullptr};
  Node b{2, nullptr};
  a.next = &b;
  b.next = &a;

  std::vector<int> visited;
  auto visitor = ae::Override{[&](int v) { visited.push_back(v); },
                              [&](Node&) { return true; }};

  ae::domain_visitor::DomainVisit(
      a, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::VisitPolicy::kAny>{});

  TEST_ASSERT_EQUAL(2, static_cast<int>(visited.size()));
  TEST_ASSERT_EQUAL(1, visited.at(0));
  TEST_ASSERT_EQUAL(2, visited.at(1));
}

// Verifies raw pointer holders are excluded from cycle tracking.
void test_CycleTracksRawPointerPointees() {
  Node a{1, nullptr};
  Node b{2, nullptr};
  a.next = &b;
  b.next = &a;
  Node* root = &a;
  auto callback = [](int) {};
  auto visitor =
      ae::domain_visitor::DomainNodeVisitor<decltype(callback),
                                            ae::domain_visitor::kAny>{callback};
  auto cycle_detector = ae::domain_visitor::CycleDetector{};

  ae::domain_visitor::DomainVisit(cycle_detector, root, std::move(visitor));

  TEST_ASSERT_EQUAL(2, static_cast<int>(cycle_detector.visited_objects.size()));
}

// Verifies the default policy visits immediate reflectable fields.
void test_DefaultPolicyVisitsReflectableFieldsShallowly() {
  Wrapper wrapper{{kExpectedVisitedValue, nullptr}};
  int visited = 0;

  ae::domain_visitor::DomainVisit(wrapper,
                                  ae::Override{[&](Node&) { ++visited; }});

  TEST_ASSERT_EQUAL(1, visited);
}

// Verifies exact shallow visits only the immediate reflected field.
void test_ExactShallowDoesNotRecurse() {
  Root root{{kExpectedVisitedValue}};
  int leaf_count = 0;
  int value_count = 0;
  auto visitor =
      ae::Override{[&](Leaf&) { ++leaf_count; }, [&](int) { ++value_count; }};

  ae::domain_visitor::DomainVisit(
      root, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kShallow>{});

  TEST_ASSERT_EQUAL(1, leaf_count);
  TEST_ASSERT_EQUAL(0, value_count);
}

// Verifies shallow does not block the enabled reflection category.
void test_ShallowAndReflectionRecurse() {
  Root root{{kExpectedVisitedValue}};
  int value_count = 0;
  auto visitor = ae::Override{[&](int) { ++value_count; }};

  ae::domain_visitor::DomainVisit(
      root, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kShallow |
                                      ae::domain_visitor::kReflection>{});

  TEST_ASSERT_EQUAL(1, value_count);
}

// Verifies shallow does not block the enabled deep category.
void test_ShallowAndDeepRecurse() {
  std::vector<std::vector<int>> values{{kExpectedDeepVisitedValue}};
  int vector_visited = 0;
  int value_visited = 0;
  auto visitor =
      ae::Override{[&](std::vector<int> const& value) {
                     ++vector_visited;
                     TEST_ASSERT_EQUAL(kExpectedDeepVisitedValue, value.at(0));
                   },
                   [&](int value) {
                     ++value_visited;
                     TEST_ASSERT_EQUAL(kExpectedDeepVisitedValue, value);
                   }};

  ae::domain_visitor::DomainVisit(
      values, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kShallow |
                                      ae::domain_visitor::kDeep>{});

  TEST_ASSERT_EQUAL(1, vector_visited);
  TEST_ASSERT_EQUAL(1, value_visited);
}

// Verifies shallow does not block enabled pointer and reflection categories.
void test_ShallowPointersAndReflectionRecurse() {
  Node node{kExpectedVisitedValue, nullptr};
  Node* node_ptr = &node;
  int value_count = 0;
  auto visitor = ae::Override{[&](int) { ++value_count; }};

  ae::domain_visitor::DomainVisit(
      node_ptr, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kShallow |
                                      ae::domain_visitor::kPointers |
                                      ae::domain_visitor::kReflection>{});

  TEST_ASSERT_EQUAL(1, value_count);
}

// Verifies a false reference callback stops recursive traversal.
void test_FalseStopsRecursion() {
  Node a{1, nullptr};
  Node b{2, nullptr};
  a.next = &b;

  std::vector<int> visited;
  int next_callback_count = 0;
  auto visitor = ae::Override{[&](int v) { visited.push_back(v); },
                              [&](Node* next) {
                                ++next_callback_count;
                                TEST_ASSERT_EQUAL_PTR(&b, next);
                                return false;
                              }};

  ae::domain_visitor::DomainVisit(
      a, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, static_cast<int>(visited.size()));
  TEST_ASSERT_EQUAL(1, visited.at(0));
  TEST_ASSERT_EQUAL(1, next_callback_count);
}

// Verifies null pointers do not invoke the reference fallback callback.
void test_NullPointerDoesNotDereferenceReferenceFallback() {
  Node a{1, nullptr};

  std::vector<int> visited;
  int fallback_count = 0;
  auto visitor = ae::Override{[&](int v) { visited.push_back(v); },
                              [&](Node&) {
                                ++fallback_count;
                                return true;
                              }};

  ae::domain_visitor::DomainVisit(
      a, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, static_cast<int>(visited.size()));
  TEST_ASSERT_EQUAL(1, visited.at(0));
  TEST_ASSERT_EQUAL(0, fallback_count);
}

// Verifies a null pointer invokes its pointer callback without dereferencing.
void test_NullPointerInvokesPointerCallbackOnce() {
  Node a{1, nullptr};

  int pointer_count = 0;
  int fallback_count = 0;
  auto visitor = ae::Override{[&](Node* node) {
                                ++pointer_count;
                                return node != nullptr;
                              },
                              [&](Node&) { ++fallback_count; }};

  ae::domain_visitor::DomainVisit(
      a, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, pointer_count);
  TEST_ASSERT_EQUAL(0, fallback_count);
}

// Verifies a pointer callback controls recursion before reference fallback.
void test_PointerOverloadControlsRecursion() {
  Node a{1, nullptr};
  Node b{2, nullptr};
  a.next = &b;

  std::vector<int> visited;
  int pointer_count = 0;
  int fallback_count = 0;
  auto visitor = ae::Override{[&](int v) { visited.push_back(v); },
                              [&](Node* node) {
                                ++pointer_count;
                                return node == nullptr;
                              },
                              [&](Node&) {
                                ++fallback_count;
                                return true;
                              }};

  ae::domain_visitor::DomainVisit(
      a, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, pointer_count);
  TEST_ASSERT_EQUAL(0, fallback_count);
  TEST_ASSERT_EQUAL(1, static_cast<int>(visited.size()));
  TEST_ASSERT_EQUAL(1, visited.at(0));
}

// Verifies a true raw pointer callback permits traversal of the pointee.
void test_NonNullPointerCallbackTrueTraversesPointee() {
  Node a{1, nullptr};
  Node b{2, nullptr};
  a.next = &b;

  std::vector<int> visited;
  int pointer_count = 0;
  auto visitor = ae::Override{[&](int value) { visited.push_back(value); },
                              [&](Node*) {
                                ++pointer_count;
                                return true;
                              }};

  ae::domain_visitor::DomainVisit(
      a, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(2, pointer_count);
  TEST_ASSERT_EQUAL(2, static_cast<int>(visited.size()));
  TEST_ASSERT_EQUAL(1, visited.at(0));
  TEST_ASSERT_EQUAL(2, visited.at(1));
}

// Verifies a direct null pointer does not invoke reference fallback.
void test_DirectNullPointerDoesNotInvokeReferenceFallback() {
  Node* node = nullptr;

  int fallback_count = 0;
  auto visitor = ae::Override{[&](Node&) { ++fallback_count; }};

  ae::domain_visitor::DomainVisit(
      node, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(0, fallback_count);
}

// Verifies empty smart pointers invoke their wrapper callbacks without a
// pointee traversal.
void test_EmptySmartPointersDoNotTraversePointees() {
  SmartPointerRoot root;
  int unique_pointer_count = 0;
  int shared_pointer_count = 0;
  int value_count = 0;
  auto visitor = ae::Override{[&](std::unique_ptr<Node> const& pointer) {
                                ++unique_pointer_count;
                                TEST_ASSERT_NULL(pointer.get());
                              },
                              [&](std::shared_ptr<Node> const& pointer) {
                                ++shared_pointer_count;
                                TEST_ASSERT_NULL(pointer.get());
                              },
                              [&](int) { ++value_count; }};

  ae::domain_visitor::DomainVisit(
      root, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, unique_pointer_count);
  TEST_ASSERT_EQUAL(1, shared_pointer_count);
  TEST_ASSERT_EQUAL(0, value_count);
}

// Verifies a false smart-pointer wrapper callback prevents dereferencing it.
void test_SmartPointerCallbackFalsePreventsDereference() {
  SmartPointerRoot root{
      std::make_unique<Node>(Node{kExpectedVisitedValue}),
      std::make_shared<Node>(Node{kExpectedDeepVisitedValue})};
  int unique_pointer_count = 0;
  int shared_pointer_count = 0;
  int value_count = 0;
  auto visitor = ae::Override{[&](std::unique_ptr<Node> const&) {
                                ++unique_pointer_count;
                                return false;
                              },
                              [&](std::shared_ptr<Node> const&) {
                                ++shared_pointer_count;
                                return false;
                              },
                              [&](int) { ++value_count; }};

  ae::domain_visitor::DomainVisit(
      root, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, unique_pointer_count);
  TEST_ASSERT_EQUAL(1, shared_pointer_count);
  TEST_ASSERT_EQUAL(0, value_count);
}

// Verifies smart-pointer traversal reaches a pointee's pointer field and a
// false child callback stops before traversing its non-null pointee.
void test_SmartPointerPointeeCallbackFalseStopsChildTraversal() {
  auto root = std::make_shared<SmartNode>(SmartNode{
      kExpectedVisitedValue,
      std::make_shared<SmartNode>(SmartNode{
          kExpectedDeepVisitedValue,
          std::make_shared<SmartNode>(SmartNode{kExpectedVisitedValue})})});
  std::vector<int> visited;
  int smart_pointer_count = 0;
  auto visitor = ae::Override{[&](int value) { visited.push_back(value); },
                              [&](std::shared_ptr<SmartNode> const&) {
                                return ++smart_pointer_count == 1;
                              }};

  ae::domain_visitor::DomainVisit(
      root, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(2, static_cast<int>(visited.size()));
  TEST_ASSERT_EQUAL(kExpectedVisitedValue, visited.at(0));
  TEST_ASSERT_EQUAL(kExpectedDeepVisitedValue, visited.at(1));
  TEST_ASSERT_EQUAL(2, smart_pointer_count);
}

// Verifies reference wrappers preserve mutable and const pointee dispatch.
void test_ReferenceWrapperPreservesPointeeCvDispatch() {
  Node mutable_node{kExpectedVisitedValue};
  std::reference_wrapper<Node> const mutable_node_reference{mutable_node};
  Node const const_node{kExpectedVisitedValue};
  std::reference_wrapper<Node const> const const_node_reference{const_node};
  int mutable_value_count = 0;
  int const_value_count = 0;
  auto visitor = ae::Override{[&](int&) { ++mutable_value_count; },
                              [&](int const&) { ++const_value_count; }};

  ae::domain_visitor::DomainVisit(
      mutable_node_reference, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});
  ae::domain_visitor::DomainVisit(
      const_node_reference, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, mutable_value_count);
  TEST_ASSERT_EQUAL(1, const_value_count);
}

// Verifies const pointees retain their cv qualification through pointer-like
// traversal.
void test_ConstSmartPointeesDispatchConstValues() {
  auto const_node = std::make_shared<Node const>(Node{kExpectedVisitedValue});
  std::unique_ptr<Node const> unique_node =
      std::make_unique<Node const>(Node{kExpectedDeepVisitedValue});
  Node const* raw_node = const_node.get();
  int mutable_value_count = 0;
  int const_value_count = 0;
  auto visitor = ae::Override{[&](int&) { ++mutable_value_count; },
                              [&](int const&) { ++const_value_count; }};

  ae::domain_visitor::DomainVisit(
      const_node, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});
  ae::domain_visitor::DomainVisit(
      unique_node, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});
  ae::domain_visitor::DomainVisit(
      raw_node, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(0, mutable_value_count);
  TEST_ASSERT_EQUAL(3, const_value_count);
}

// Verifies const reference-wrapper pointees dispatch only const field
// callbacks.
void test_ConstReferenceWrapperPointeeDispatchesConstValues() {
  Node const node{kExpectedVisitedValue};
  std::reference_wrapper<Node const> node_reference{node};
  int mutable_value_count = 0;
  int const_value_count = 0;
  auto visitor = ae::Override{[&](int&) { ++mutable_value_count; },
                              [&](int const&) { ++const_value_count; }};

  ae::domain_visitor::DomainVisit(
      node_reference, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(0, mutable_value_count);
  TEST_ASSERT_EQUAL(1, const_value_count);
}

// Verifies aliasing shared pointers visit their distinct wrappers but traverse
// their common pointee only once.
void test_AliasingSharedPointersVisitCommonPointeeOnce() {
  auto owner = std::make_shared<Node>(Node{kExpectedVisitedValue});
  std::shared_ptr<Node> alias(owner, owner.get());
  AliasingRoot aliasing_root{owner, alias};
  int wrapper_count = 0;
  int value_count = 0;
  auto visitor =
      ae::Override{[&](std::shared_ptr<Node> const&) { ++wrapper_count; },
                   [&](int) { ++value_count; }};

  ae::domain_visitor::DomainVisit(
      aliasing_root, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(2, wrapper_count);
  TEST_ASSERT_EQUAL(1, value_count);
}

// Verifies a shared-pointer cycle is terminated after visiting its root value.
void test_SharedPointerCycleTerminates() {
  auto node = std::make_shared<SmartNode>(SmartNode{kExpectedVisitedValue});
  node->next = node;
  int value_count = 0;
  auto visitor = ae::Override{[&](int value) {
    TEST_ASSERT_EQUAL(kExpectedVisitedValue, value);
    ++value_count;
  }};

  ae::domain_visitor::DomainVisit(
      node, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, value_count);
}

// Verifies exact shallow visits a smart-pointer field without dereferencing it,
// while mixed shallow pointer and reflection policies recurse.
void test_ExactShallowDoesNotDereferenceSmartPointers() {
  SmartPointerRoot shallow_root{
      std::make_unique<Node>(Node{kExpectedVisitedValue}), nullptr};
  SmartPointerRoot mixed_root{
      std::make_unique<Node>(Node{kExpectedDeepVisitedValue}), nullptr};
  int shallow_wrapper_count = 0;
  int shallow_value_count = 0;
  auto shallow_visitor = ae::Override{
      [&](std::unique_ptr<Node> const&) { ++shallow_wrapper_count; },
      [&](int) { ++shallow_value_count; }};

  ae::domain_visitor::DomainVisit(
      shallow_root, shallow_visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kShallow>{});
  TEST_ASSERT_EQUAL(1, shallow_wrapper_count);
  TEST_ASSERT_EQUAL(0, shallow_value_count);

  int mixed_value_count = 0;
  auto mixed_visitor = ae::Override{[&](int) { ++mixed_value_count; }};
  ae::domain_visitor::DomainVisit(
      mixed_root, mixed_visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kShallow |
                                      ae::domain_visitor::kReflection |
                                      ae::domain_visitor::kPointers>{});

  TEST_ASSERT_EQUAL(1, mixed_value_count);
}

// Verifies public policy constants and named visitor traversal are usable.
void test_PublicPoliciesAndNamedDomainVisitor() {
  static_assert(ae::domain_visitor::kDeep == 4);
  static_assert(ae::domain_visitor::kAny != 0);

  Wrapper wrapper{{kExpectedVisitedValue, nullptr}};
  int visited = 0;
  auto callback = [&](int v) { visited = v; };
  auto visitor = ae::domain_visitor::DomainNodeVisitor<
      decltype(callback), ae::domain_visitor::kReflection>{callback};
  ae::domain_visitor::DomainVisit(wrapper, visitor);
  TEST_ASSERT_EQUAL(kExpectedVisitedValue, visited);
}

// Verifies a named lvalue-held wrapper remains usable throughout recursion.
void test_NamedLvalueHeldVisitorRecurses() {
  Node a{1, nullptr};
  Node b{2, nullptr};
  a.next = &b;
  int visits = 0;
  auto callback = [&](int) { ++visits; };
  auto visitor = ae::domain_visitor::DomainNodeVisitor<
      decltype(callback)&, ae::domain_visitor::kReflection |
                               ae::domain_visitor::kPointers>{callback};

  ae::domain_visitor::DomainVisit(a, visitor);

  TEST_ASSERT_EQUAL(2, visits);
}

// Verifies const wrapper dispatch permits mutable value callback state.
void test_MutableValueCallbackState() {
  Wrapper wrapper{{kExpectedVisitedValue, nullptr}};
  auto visitor =
      ae::domain_visitor::DomainNodeVisitor<MutableIntCallback,
                                            ae::domain_visitor::kReflection>{
          MutableIntCallback{}};

  ae::domain_visitor::DomainVisit(wrapper, visitor);

  TEST_ASSERT_EQUAL(1, visitor.visitor.visits);
}

// Verifies named wrappers are not copied while traversing recursively.
void test_NamedVisitorIdentityAndNoCopy() {
  Node a{1, nullptr};
  Node b{2, nullptr};
  a.next = &b;
  int visits = 0;
  NonCopyableIntCallback callback{visits};
  auto visitor = ae::domain_visitor::DomainNodeVisitor<
      NonCopyableIntCallback&, ae::domain_visitor::kReflection |
                                   ae::domain_visitor::kPointers>{callback};

  ae::domain_visitor::DomainVisit(a, visitor);

  TEST_ASSERT_TRUE(std::addressof(visitor.visitor) == std::addressof(callback));
  TEST_ASSERT_EQUAL(2, visits);
}

// Verifies move-only value callbacks traverse recursive nodes without copies.
void test_MoveOnlyCallbackTraversal() {
  Node a{1, nullptr};
  Node b{2, nullptr};
  a.next = &b;
  int visits = 0;
  auto visitor =
      ae::domain_visitor::DomainNodeVisitor<MoveOnlyIntCallback,
                                            ae::domain_visitor::kReflection |
                                                ae::domain_visitor::kPointers>{
          MoveOnlyIntCallback{visits}};

  ae::domain_visitor::DomainVisit(a, visitor);

  TEST_ASSERT_EQUAL(2, visits);
}

// Verifies owned callbacks are invoked through their const lvalue overload.
void test_OwnedCallbackInvokesConstLvalueOverload() {
  Wrapper wrapper{{kExpectedVisitedValue, nullptr}};
  auto visitor =
      ae::domain_visitor::DomainNodeVisitor<InvocationCategoryCallback,
                                            ae::domain_visitor::kReflection>{
          InvocationCategoryCallback{}};

  ae::domain_visitor::DomainVisit(wrapper, visitor);

  TEST_ASSERT_EQUAL(1, visitor.visitor.const_lvalue_visits);
  TEST_ASSERT_EQUAL(0, visitor.visitor.rvalue_visits);
}

// Verifies rvalue-reference callbacks are invoked through const lvalue
// overload.
void test_RvalueReferenceCallbackInvokesConstLvalueOverload() {
  Wrapper wrapper{{kExpectedVisitedValue, nullptr}};
  InvocationCategoryCallback callback;
  auto visitor =
      ae::domain_visitor::DomainNodeVisitor<InvocationCategoryCallback&&,
                                            ae::domain_visitor::kReflection>{
          std::move(callback)};

  ae::domain_visitor::DomainVisit(wrapper, visitor);

  TEST_ASSERT_EQUAL(1, visitor.visitor.const_lvalue_visits);
  TEST_ASSERT_EQUAL(0, visitor.visitor.rvalue_visits);
}

// Verifies reflection-only traversal visits inherited reflected fields.
void test_DomainVisitInheritedFieldsFromBaseOnlyReflection() {
  VisitDerived derived{{kExpectedVisitedValue}};
  int visited = 0;
  auto visitor = ae::Override{[&](int v) { visited = v; },
                              [&](VisitDerived&) { return true; }};

  ae::domain_visitor::DomainVisit(
      derived, visitor,
      ae::domain_visitor::PolicyConst<ae::domain_visitor::kReflection>{});
  TEST_ASSERT_EQUAL(kExpectedVisitedValue, visited);
}
}  // namespace

int test_DomainVisitor() {
  UNITY_BEGIN();
  RUN_TEST(test_CycleWithReferenceFallback);
  RUN_TEST(test_CycleTracksRawPointerPointees);
  RUN_TEST(test_DefaultPolicyVisitsReflectableFieldsShallowly);
  RUN_TEST(test_ExactShallowDoesNotRecurse);
  RUN_TEST(test_ShallowAndReflectionRecurse);
  RUN_TEST(test_ShallowAndDeepRecurse);
  RUN_TEST(test_ShallowPointersAndReflectionRecurse);
  RUN_TEST(test_FalseStopsRecursion);
  RUN_TEST(test_NullPointerDoesNotDereferenceReferenceFallback);
  RUN_TEST(test_NullPointerInvokesPointerCallbackOnce);
  RUN_TEST(test_PointerOverloadControlsRecursion);
  RUN_TEST(test_NonNullPointerCallbackTrueTraversesPointee);
  RUN_TEST(test_DirectNullPointerDoesNotInvokeReferenceFallback);
  RUN_TEST(test_EmptySmartPointersDoNotTraversePointees);
  RUN_TEST(test_SmartPointerCallbackFalsePreventsDereference);
  RUN_TEST(test_SmartPointerPointeeCallbackFalseStopsChildTraversal);
  RUN_TEST(test_ReferenceWrapperPreservesPointeeCvDispatch);
  RUN_TEST(test_ConstSmartPointeesDispatchConstValues);
  RUN_TEST(test_ConstReferenceWrapperPointeeDispatchesConstValues);
  RUN_TEST(test_AliasingSharedPointersVisitCommonPointeeOnce);
  RUN_TEST(test_SharedPointerCycleTerminates);
  RUN_TEST(test_ExactShallowDoesNotDereferenceSmartPointers);
  RUN_TEST(test_PublicPoliciesAndNamedDomainVisitor);
  RUN_TEST(test_NamedLvalueHeldVisitorRecurses);
  RUN_TEST(test_MutableValueCallbackState);
  RUN_TEST(test_NamedVisitorIdentityAndNoCopy);
  RUN_TEST(test_MoveOnlyCallbackTraversal);
  RUN_TEST(test_OwnedCallbackInvokesConstLvalueOverload);
  RUN_TEST(test_RvalueReferenceCallbackInvokesConstLvalueOverload);
  RUN_TEST(test_DomainVisitInheritedFieldsFromBaseOnlyReflection);
  return UNITY_END();
}

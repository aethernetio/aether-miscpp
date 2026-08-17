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

#ifndef AETHER_MISCPP_REFLECT_DETAILS_DEFINES_H_
#define AETHER_MISCPP_REFLECT_DETAILS_DEFINES_H_

// IWYU pragma: begin_exports
#include "aether-miscpp/reflect/details/meta.h"
#include "aether-miscpp/reflect/details/mirror.h"
#include "aether-miscpp/reflect/details/reflection.h"
// IWYU pragma: end_exports

#define _AE_APPLY_MACRO_0(MACRO, ...)
#define _AE_APPLY_MACRO_1(MACRO, ARG, ...) \
  MACRO(ARG) _AE_APPLY_MACRO_0(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_2(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_1(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_3(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_2(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_4(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_3(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_5(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_4(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_6(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_5(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_7(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_6(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_8(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_7(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_9(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_8(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_10(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_9(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_11(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_10(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_12(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_11(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_13(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_12(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_14(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_13(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_15(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_14(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_16(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_15(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_17(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_16(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_18(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_17(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_19(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_18(MACRO, __VA_ARGS__)
#define _AE_APPLY_MACRO_20(MACRO, ARG, ...) \
  MACRO(ARG), _AE_APPLY_MACRO_19(MACRO, __VA_ARGS__)

#define _AE_APPLY_MACRO_N(MACRO, _20, _19, _18, _17, _16, _15, _14, _13, _12, \
                          _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, X,    \
                          ...)                                                \
  _AE_APPLY_MACRO##X(MACRO, _20, _19, _18, _17, _16, _15, _14, _13, _12, _11, \
                     _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, _0)

#define _AE_APPLY_MACRO(MACRO, ...)                                         \
  _AE_APPLY_MACRO_N(MACRO, __VA_ARGS__, _20, _19, _18, _17, _16, _15, _14,  \
                    _13, _12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, \
                    _0)

/**
 * \brief Provide access to regular class member through pointer to member.
 */
#define AE_MMBR(MEMBER) \
  ::ae::reflect::MetaMember { #MEMBER, &SelfType::MEMBER }

/**
 * \brief Provide access to regular class member through pointer to member.
 */
#define AE_MMBRS(...) _AE_APPLY_MACRO(AE_MMBR, __VA_ARGS__)

/**
 * \brief Provide access to reference type member.
 * c++ forbids create pointers to members if it's reference type.
 */
#define AE_REF(MEMBER)                                  \
  ::ae::reflect::MetaGetter {                           \
    #MEMBER, [](SelfType* obj) { return &obj->MEMBER; } \
  }

/**
 * \brief Provide a member as reference to Base class.
 */
#define AE_REF_BASE(Base)                                        \
  ::ae::reflect::MetaGetter {                                    \
    #Base, [](SelfType* obj) { return static_cast<Base*>(obj); } \
  }

/**
 * \brief Provide a mirror for Base class.
 */
#define AE_BASE(Base) Base::template Mirror<Base>()

/**
 * \brief Make type reflectable with manual members marking.
 * Use AE_MMBR, AE_REF, AE_REF_BASE or AE_BASE to mark members.
 */
#define AE_REFLECT(...)                                               \
  template <typename SelfType>                                        \
  static consteval auto Mirror() noexcept {                           \
    static_assert(sizeof(SelfType) != 0);                             \
    return ::ae::reflect::Mirror{::ae::reflect::ClassTag<SelfType>{}, \
                                 __VA_ARGS__};                        \
  }

/**
 * \brief Make type reflectable with each listed member marked automatically as
 * class member.
 */
#define AE_REFLECT_MEMBERS(...)                                          \
  template <typename SelfType>                                           \
  static constexpr auto Mirror() noexcept {                              \
    static_assert(sizeof(SelfType) != 0);                                \
    return ::ae::reflect::Mirror{::ae::reflect::ClassTag<SelfType>{},    \
                                 _AE_APPLY_MACRO(AE_MMBR, __VA_ARGS__)}; \
  }

#endif  // AETHER_MISCPP_REFLECT_DETAILS_DEFINES_H_

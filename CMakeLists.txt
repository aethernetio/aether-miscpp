# Copyright 2026 Aethernet Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(aether-miscpp VERSION 1.0.0 LANGUAGES CXX)

# define is the project is the current root project
# root project builds tests and defines installation targets by default
if( ${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(IS_ROOT_PROJECT ON)
else()
  set(IS_ROOT_PROJECT OFF)
endif()

option(AE_BUILD_TESTS "Build tests" ${IS_ROOT_PROJECT} )
option(AE_INSTALL "Install aether-miscpp library" ${IS_ROOT_PROJECT} )

add_library(${PROJECT_NAME} INTERFACE)
add_library(aether::miscpp ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(AE_BUILD_TESTS)
  enable_testing()
  add_subdirectory("tests")
endif()

if(AE_INSTALL)
  include(GNUInstallDirs)

  # define the installation target, install lib headers and export cmake config
  install(TARGETS ${PROJECT_NAME}
          EXPORT ${PROJECT_NAME}Targets
  )

  install(DIRECTORY src/aether-miscpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(EXPORT ${PROJECT_NAME}Targets
          FILE ${PROJECT_NAME}Targets.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
endif()

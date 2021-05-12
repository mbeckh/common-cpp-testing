# Copyright 2021 Michael Beckh
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project("m4t" VERSION 0.0.1
    DESCRIPTION "Common C++ library for testing"
    HOMEPAGE_URL "https://github.com/mbeckh/m4t"
    LANGUAGES CXX)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" IS_MAIN_PROJECT)
option(BUILD_TESTING "Build tests" ${IS_MAIN_PROJECT})

find_package(GTest REQUIRED)

add_library(m4t
    "src/IStream_Mock.cpp"
    "src/MallocSpy.cpp"
    "src/memory.cpp"
    "include/m4t/IStream_Mock.h"
    "include/m4t/m4t.h"
    "include/m4t/MallocSpy.h"
    "include/m4t/memory.h"
)
add_library(m4t::m4t ALIAS m4t)

target_compile_definitions(m4t PRIVATE WIN32_LEAN_AND_MEAN=1 NOMINMAX=1)
target_compile_features(m4t PUBLIC cxx_std_20)
target_precompile_headers(m4t PRIVATE "src/pch.h")
target_include_directories(m4t PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:include>")
set_target_properties(m4t PROPERTIES
    DEBUG_POSTFIX d
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_link_libraries(m4t PUBLIC GTest::gmock propsys)

include(CMakePackageConfigHelpers)
configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/m4t-config.cmake.in" "${PROJECT_BINARY_DIR}/m4t-config.cmake" INSTALL_DESTINATION "share/m4t")
write_basic_package_version_file("${PROJECT_BINARY_DIR}/m4t-config-version.cmake" VERSION ${m4t_VERSION} COMPATIBILITY SameMajorVersion)

install(TARGETS m4t EXPORT m4t-targets)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/m4t" TYPE INCLUDE)
install(EXPORT m4t-targets DESTINATION "share/m4t" NAMESPACE "m4t::")
install(FILES "${PROJECT_BINARY_DIR}/m4t-config.cmake" "${PROJECT_BINARY_DIR}/m4t-config-version.cmake" DESTINATION "share/m4t")

if(BUILD_TESTING)
	include(GoogleTest)
	enable_testing()

    add_executable(m4t_Test
        "test/m4t_Test.cpp"
        "test/IStream_Mock_Test.cpp"
        "test/MallocSpy_Test.cpp"
    )

    target_compile_definitions(m4t PRIVATE WIN32_LEAN_AND_MEAN=1 NOMINMAX=1)
    target_compile_features(m4t PUBLIC cxx_std_20)
    target_precompile_headers(m4t_Test PRIVATE "test/pch.h")
    set_target_properties(m4t_Test PROPERTIES
        DEBUG_POSTFIX d
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    target_link_libraries(m4t_Test PRIVATE m4t::m4t GTest::gmock GTest::gmock_main)

    gtest_discover_tests(m4t_Test)
endif()

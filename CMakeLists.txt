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
cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project("common-cpp-testing" VERSION 0.0.3
    DESCRIPTION "Common C++ library for testing"
    HOMEPAGE_URL "https://github.com/mbeckh/common-cpp-testing"
    LANGUAGES CXX)

find_package(GTest REQUIRED)
find_package(detours-gmock REQUIRED)

add_library(m4t
    "src/IStreamMock.cpp"
    "src/LogListener.cpp"
    "src/m4t.cpp"
    "src/MallocSpy.cpp"
    "include/m4t/IStreamMock.h"
    "include/m4t/LogListener.h"
    "include/m4t/m4t.h"
    "include/m4t/MallocSpy.h"
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
target_link_libraries(m4t PUBLIC GTest::gmock PRIVATE detours-gmock::detours-gmock propsys)

include(CMakePackageConfigHelpers)
configure_package_config_file("cmake/m4t-config.cmake.in" "m4t-config.cmake" INSTALL_DESTINATION "share/common-cpp-testing")
write_basic_package_version_file("${PROJECT_BINARY_DIR}/m4t-config-version.cmake" VERSION ${m4t_VERSION} COMPATIBILITY SameMajorVersion)

install(TARGETS m4t EXPORT m4t-targets)
install(DIRECTORY "include/m4t" TYPE INCLUDE)
install(EXPORT m4t-targets DESTINATION "share/common-cpp-testing" NAMESPACE "m4t::")
install(FILES "${PROJECT_BINARY_DIR}/m4t-config.cmake" "${PROJECT_BINARY_DIR}/m4t-config-version.cmake" DESTINATION "share/common-cpp-testing")

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
	enable_testing()

    add_executable(m4t_Test
        "test/IStreamMock.test.cpp"
        "test/LogListener.test.cpp"
        "test/m4t.test.cpp"
        "test/MallocSpy.test.cpp"
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

    add_test(NAME m4t_Test_PASS COMMAND m4t_Test)
endif()

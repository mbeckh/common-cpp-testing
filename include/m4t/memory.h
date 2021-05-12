/*
Copyright 2020 Michael Beckh

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/// @file

#pragma once

#include <cstddef>

namespace m4t {

void memory_start_tracking(const void* ptr);  // NOLINT(readability-identifier-naming)
void memory_stop_tracking();                  // NOLINT(readability-identifier-naming)
bool memory_is_deleted(const void* ptr);      // NOLINT(readability-identifier-naming)

namespace internal {

void* memory_new(std::size_t count);     // NOLINT(readability-identifier-naming)
void memory_delete(void* ptr) noexcept;  // NOLINT(readability-identifier-naming)

}  // namespace internal

}  // namespace m4t

/// @brief Use the tracking handlers for `new` and `delete`.
/// @note The macro MUST be called exactly once in the whole code base.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INSTALL_TRACKING_NEW_DELETE()                                                                               \
	void* operator new(const std::size_t count) { /* NOLINT(readability-inconsistent-declaration-parameter-name) */ \
		return m4t::internal::memory_new(count);                                                                    \
	}                                                                                                               \
	void operator delete(void* ptr) noexcept { /* NOLINT(readability-inconsistent-declaration-parameter-name) */    \
		m4t::internal::memory_delete(ptr);                                                                          \
	}

// https://msdn.microsoft.com/en-us/library/aa270812(v=vs.60).aspx
// https://msdn.microsoft.com/de-de/library/974tc9t1.aspx
// https://en.wikipedia.org/wiki/Magic_number_(programming)
/// @brief Check if a memory address contains uninitialized data.
/// @note Requires the use of `INSTALL_TRACKING_NEW_DELETE()`.
/// @param p_ The address to check.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_UNINITIALIZED(p_) __pragma(warning(suppress : 6001)) EXPECT_EQ(0xCDCDCDCD, *std::bit_cast<std::uint32_t*>((p_)))

/// @brief Check if a memory address contains deleted data.
/// @note Requires the use of `INSTALL_TRACKING_NEW_DELETE()`.
/// @param p_ The address to check.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_DELETED(p_) EXPECT_TRUE(m4t::memory_is_deleted((p_)))

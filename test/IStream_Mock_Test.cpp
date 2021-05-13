/*
Copyright 2021 Michael Beckh

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

#include "m4t/IStream_Mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <combaseapi.h>
#include <objidl.h>
#include <windows.h>
#include <wtypes.h>

namespace m4t::test {

namespace t = testing;

TEST(IStream_Mock, Stat) {  // NOLINT(cert-err58-cpp, cppcoreguidelines-avoid-non-const-global-variables)
	IStream_Mock mock;

	EXPECT_CALL(mock, Stat(t::_, STATFLAG_DEFAULT))
		.WillOnce(IStream_Stat(L"Test.txt"));

	STATSTG stg;
	ASSERT_HRESULT_SUCCEEDED(mock.Stat(&stg, STATFLAG_DEFAULT));
	ASSERT_STREQ(L"Test.txt", stg.pwcsName);

	CoTaskMemFree(stg.pwcsName);
}

}  // namespace m4t::test

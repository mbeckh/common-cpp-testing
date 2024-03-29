/*
Copyright 2021-2022 Michael Beckh

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

#include "m4t/m4t.h"

#include "m4t/IStreamMock.h"

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>  // IWYU pragma: keep
#include <gtest/gtest.h>

#include <windows.h>
#include <combaseapi.h>
#include <oaidl.h>
#include <objidl.h>
#include <propidl.h>
#include <propvarutil.h>
#include <unknwn.h>
#include <wtypes.h>

#include <cstdint>
#include <regex>
#include <string>
#include <system_error>

namespace m4t::test {
namespace {

//
// Helpers
//

TEST(m4t, AssertExpect) {
	static int value = 1;

	ASSERT_NULL(nullptr);
	EXPECT_FATAL_FAILURE(ASSERT_NULL(&value), "Expected equality");

	EXPECT_FATAL_FAILURE(ASSERT_NOT_NULL(nullptr), "(nullptr) !=");
	ASSERT_NOT_NULL(&value);

	EXPECT_NULL(nullptr);
	EXPECT_NONFATAL_FAILURE(EXPECT_NULL(&value), "Expected equality");

	EXPECT_NONFATAL_FAILURE(EXPECT_NOT_NULL(nullptr), "(nullptr) !=");
	EXPECT_NOT_NULL(&value);
}

//
// Locale
//

TEST(m4t, HasLocale_EnglishUS_ReturnTrue) {
	EXPECT_TRUE(HasLocale("en-US"));
}

TEST(m4t, HasLocale_Swahili_ReturnFalse) {
	// sorry to all folks using a Swahili localization... :-)
	EXPECT_FALSE(HasLocale("sw"));
}

TEST(m4t, WithLocale_EnglishUS_IsEnglish) {
	constexpr auto kLambda = [] {
		return std::system_category().message(ERROR_ACCESS_DENIED);
	};
	const std::string withDefaultLanguage = kLambda();

	const std::string str = WithLocale("en-US", kLambda);
	EXPECT_EQ("Access is denied.", str);

	// check that default is in effect after change
	EXPECT_EQ(withDefaultLanguage, kLambda());
}

TEST(m4t, WithLocale_GermanGermany_IsGerman) {
	if (!HasLocale("de-DE")) {
		// account for German not being available on GitHub hosted runners
		GTEST_SKIP();
		return;
	}

	constexpr auto kLambda = [] {
		return std::system_category().message(ERROR_ACCESS_DENIED);
	};
	const std::string withDefaultLanguage = kLambda();

	const std::string str = WithLocale("de-DE", kLambda);
	EXPECT_EQ("Zugriff verweigert", str);

	// check that default is in effect after change
	EXPECT_EQ(withDefaultLanguage, kLambda());
}

//
// COM Mock
//

TEST(m4t, ComMock) {
	COM_MOCK_DECLARE(mock, IStreamMock);

	COM_MOCK_SETUP(mock, IStream);
	COM_MOCK_EXPECT_REFCOUNT(1, mock);

	EXPECT_EQ(2, mock.AddRef());
	COM_MOCK_EXPECT_REFCOUNT(2, mock);

	EXPECT_EQ(1, mock.Release());
	COM_MOCK_EXPECT_REFCOUNT(1, mock);

	IDispatch* pDispatch = nullptr;
	EXPECT_EQ(E_NOINTERFACE, mock.QueryInterface(IID_PPV_ARGS(&pDispatch)));

	IStream* pStream = nullptr;
	EXPECT_HRESULT_SUCCEEDED(mock.QueryInterface(IID_PPV_ARGS(&pStream)));
	ASSERT_EQ(&mock, pStream);
	COM_MOCK_EXPECT_REFCOUNT(2, mock);

	IUnknown* pUnknown = nullptr;
	EXPECT_HRESULT_SUCCEEDED(mock.QueryInterface(IID_PPV_ARGS(&pUnknown)));
	ASSERT_EQ(&mock, pUnknown);
	COM_MOCK_EXPECT_REFCOUNT(3, mock);

	EXPECT_EQ(2, pStream->Release());
	COM_MOCK_EXPECT_REFCOUNT(2, mock);

	EXPECT_EQ(1, pUnknown->Release());
	COM_MOCK_EXPECT_REFCOUNT(1, mock);

	COM_MOCK_VERIFY(mock);
}


//
// Matchers
//

TEST(m4t, BitsSet) {
	EXPECT_THAT(4, BitsSet(4));
	EXPECT_THAT(5, BitsSet(4));
	EXPECT_THAT(7, BitsSet(6));

	EXPECT_NONFATAL_FAILURE(EXPECT_THAT(0, BitsSet(6)), "bits set");
	EXPECT_NONFATAL_FAILURE(EXPECT_THAT(4, BitsSet(6)), "bits set");
}

TEST(m4t, MatchesRegex) {
	EXPECT_THAT("abcd", MatchesRegex(std::regex(".Bx?C.", std::regex::icase)));
	EXPECT_THAT(L"abcd", MatchesRegex(std::wregex(L"^.Bx?C.$", std::regex::icase)));
	EXPECT_THAT("abcd", MatchesRegex(".bx?c."));
	EXPECT_THAT(L"abcd", MatchesRegex(L"^.bx?c.$"));

	EXPECT_NONFATAL_FAILURE(EXPECT_THAT("abcd", MatchesRegex(std::regex("bc"))), "matches regex");
	EXPECT_NONFATAL_FAILURE(EXPECT_THAT("abcd", MatchesRegex("bc")), "matches regex");
	EXPECT_NONFATAL_FAILURE(EXPECT_THAT("abcd", MatchesRegex("^bc$")), "matches regex");
}

TEST(m4t, ContainsRegex) {
	EXPECT_THAT("abcd", ContainsRegex(std::regex(".Bx?C.", std::regex::icase)));
	EXPECT_THAT(L"abcd", ContainsRegex(std::wregex(L"^.Bx?C.$", std::regex::icase)));
	EXPECT_THAT("abcd", ContainsRegex(".bx?c."));
	EXPECT_THAT(L"abcd", ContainsRegex(L"^.bx?c.$"));

	EXPECT_THAT("abcd", ContainsRegex(std::regex("bc")));
	EXPECT_THAT("abcd", ContainsRegex("bc"));
	EXPECT_NONFATAL_FAILURE(EXPECT_THAT("abcd", ContainsRegex("^bc$")), "contains regex");
}

TEST(m4t, PointerAs) {
	const char sz[] = "Test Test Test";
	const void* const ptr = sz;
	EXPECT_THAT(ptr, PointerAs<char>(t::StrEq("Test Test Test")));

	EXPECT_NONFATAL_FAILURE(EXPECT_THAT(ptr, PointerAs<char>(t::StrEq("abcd"))), "is a pointer of type char* that");
}

TEST(m4t, PointeeAs) {
	const char sz[] = "Test Test Test";
	const void* const ptr = sz;
	EXPECT_THAT(ptr, PointeeAs<char>('T'));

	EXPECT_NONFATAL_FAILURE(EXPECT_THAT(ptr, PointeeAs<char>('X')), "points to a value of type char that");
}


//
// Actions
//

TEST(m4t, SetLastError) {
	constexpr DWORD kErrorCode = 99u;

	t::MockFunction<int()> function;
	EXPECT_CALL(function, Call)
	    .WillOnce(t::DoAll(m4t::SetLastError(kErrorCode), t::Return(-1)));

	::SetLastError(0);
	EXPECT_EQ(-1, function.Call());
	EXPECT_EQ(kErrorCode, GetLastError());
}

TEST(m4t, SetLastErrorAndReturn) {
	constexpr DWORD kErrorCode = 99;

	t::MockFunction<int()> function;
	EXPECT_CALL(function, Call)
	    .WillOnce(SetLastErrorAndReturn(kErrorCode, -1));

	::SetLastError(0);
	EXPECT_EQ(-1, function.Call());
	EXPECT_EQ(kErrorCode, GetLastError());
}

TEST(m4t, SetComObject) {
	COM_MOCK_DECLARE(mock, IStreamMock);
	COM_MOCK_SETUP(mock, IStream);

	t::MockFunction<void(IUnknown**)> function;
	EXPECT_CALL(function, Call)
	    .WillOnce(SetComObject<0>(&mock));

	IUnknown* pUnknown = nullptr;
	function.Call(&pUnknown);

	ASSERT_EQ(&mock, pUnknown);
	COM_MOCK_EXPECT_REFCOUNT(2, mock);

	pUnknown->Release();
	COM_MOCK_VERIFY(mock);
}

TEST(m4t, SetPropVariant) {
	constexpr std::uint32_t kUInt32Value = 75;

	COM_MOCK_DECLARE(mock, IStreamMock);
	COM_MOCK_SETUP(mock, IStream);

	t::MockFunction<void(PROPVARIANT*, PROPVARIANT*, PROPVARIANT*, PROPVARIANT*, PROPVARIANT*)> function;
	EXPECT_CALL(function, Call)
	    .WillOnce(t::DoAll(
	        SetPropVariantToBool<0>(VARIANT_TRUE),
	        SetPropVariantToBSTR<1>(L"Test"),
	        SetPropVariantToEmpty<2>(),
	        SetPropVariantToStream<3>(&mock),
	        SetPropVariantToUInt32<4>(kUInt32Value)));

	PROPVARIANT pvBool;
	PROPVARIANT pvBstr;
	PROPVARIANT pvEmpty;
	PROPVARIANT pvStream;
	PROPVARIANT pvUInt32;

	PropVariantInit(&pvBool);
	PropVariantInit(&pvBstr);
	InitPropVariantFromBoolean(TRUE, &pvEmpty);
	PropVariantInit(&pvStream);
	PropVariantInit(&pvUInt32);

	function.Call(&pvBool, &pvBstr, &pvEmpty, &pvStream, &pvUInt32);

	EXPECT_EQ(VARENUM::VT_BOOL, pvBool.vt);
	EXPECT_EQ(VARIANT_TRUE, pvBool.boolVal);

	ASSERT_EQ(VARENUM::VT_BSTR, pvBstr.vt);
	EXPECT_STREQ(L"Test", pvBstr.bstrVal);

	ASSERT_EQ(VARENUM::VT_EMPTY, pvEmpty.vt);

	EXPECT_EQ(VARENUM::VT_STREAM, pvStream.vt);
	EXPECT_EQ(&mock, pvStream.pStream);
	COM_MOCK_EXPECT_REFCOUNT(2, mock);

	EXPECT_EQ(VARENUM::VT_UI4, pvUInt32.vt);
	EXPECT_EQ(kUInt32Value, pvUInt32.ulVal);

	PropVariantClear(&pvBool);
	PropVariantClear(&pvBstr);
	PropVariantClear(&pvEmpty);
	PropVariantClear(&pvStream);
	PropVariantClear(&pvUInt32);

	COM_MOCK_VERIFY(mock);
}

}  // namespace
}  // namespace m4t::test

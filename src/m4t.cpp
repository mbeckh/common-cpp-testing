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

#include <windows.h>
#include <detours_gmock.h>

#include <memory>
#include <mutex>
#include <string>
#include <system_error>

namespace m4t {

namespace internal {

namespace {

#undef WIN32_FUNCTIONS
#define WIN32_FUNCTIONS(fn_)                                                 \
	fn_(4, int, WINAPI, GetLocaleInfoEx,                                     \
	    (LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData), \
	    (lpLocaleName, LCType, lpLCData, cchData),                           \
	    nullptr)

/// @brief Workaround for https://github.com/microsoft/STL/issues/2882.
class GetLocaleInfoExDetour {
public:
	explicit GetLocaleInfoExDetour(std::string locale)
	    : m_locale(std::move(locale)) {
		SetUp();
	}

private:
	static std::recursive_mutex& GetDetourMutex() {
		static std::recursive_mutex detourMutex;
		return detourMutex;
	}

public:
	static void SetLocale(const std::string& locale) {
		const std::scoped_lock lock(GetDetourMutex());
		if (s_detourRefCount == 0) {
			s_detour = std::make_unique<GetLocaleInfoExDetour>(locale);
		} else {
			ASSERT_EQ(s_detour->m_locale, locale);
		}
		++s_detourRefCount;
	}

	static void Release() {
		const std::scoped_lock lock(GetDetourMutex());
		--s_detourRefCount;
		if (s_detourRefCount == 0) {
			s_detour.reset();
		}
	}

private:
	void SetUp() {
		const std::wstring name(m_locale.cbegin(), m_locale.cend());
		const LCID lcid = LocaleNameToLCID(name.c_str(), 0);
		ASSERT_NE(static_cast<LCID>(0), lcid);

		const DWORD langId = LANGIDFROMLCID(lcid);

		constexpr LPCWSTR kName = LOCALE_NAME_SYSTEM_DEFAULT;
		constexpr LCTYPE kFlags = LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER;
		constexpr int kSize = sizeof(DWORD) / sizeof(wchar_t);

		ON_CALL(m_mock, GetLocaleInfoEx(kName, kFlags, t::_, kSize))
		    .WillByDefault([langId](LPCWSTR, LCTYPE, LPWSTR lpLCData, int) {
			    std::memcpy(lpLCData, &langId, sizeof(langId));
			    return kSize;
		    });
		EXPECT_CALL(m_mock, GetLocaleInfoEx(kName, kFlags, t::_, kSize))
		    .Times(t::AnyNumber());
	}

private:
	static inline std::unique_ptr<GetLocaleInfoExDetour> s_detour;
	static inline int s_detourRefCount = 0;

private:
	DTGM_API_MOCK(m_mock, WIN32_FUNCTIONS);
	const std::string m_locale;
};

}  // namespace

void LocaleSetter::SetUp(const std::string& locale) {
	ULONG bufferSize = 0;
	ASSERT_TRUE(GetThreadPreferredUILanguages(MUI_LANGUAGE_NAME | MUI_THREAD_LANGUAGES, &m_num, nullptr, &bufferSize));

	m_buffer = std::make_unique_for_overwrite<wchar_t[]>(bufferSize);
	ASSERT_TRUE(GetThreadPreferredUILanguages(MUI_LANGUAGE_NAME | MUI_THREAD_LANGUAGES, &m_num, m_buffer.get(), &bufferSize));

	std::wstring names(locale.cbegin(), locale.cend());
	names.push_back(L'\0');
	ULONG num = 1;
	ASSERT_TRUE(SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, names.c_str(), &num));

	GetLocaleInfoExDetour::SetLocale(locale);
}

void LocaleSetter::TearDown() {
	GetLocaleInfoExDetour::Release();
	ASSERT_TRUE(SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, m_buffer.get(), &m_num));
}

}  // namespace internal


bool HasLocale(const std::string& locale) {
	std::wstring names(locale.cbegin(), locale.cend());
	names.push_back(L'\0');

	DWORD fallbackSize = 0;
	DWORD attributes = 0;
	if (!GetUILanguageInfo(MUI_LANGUAGE_NAME, names.c_str(), nullptr, &fallbackSize, &attributes)) {
		const DWORD lastError = GetLastError();
		if (lastError != ERROR_OBJECT_NOT_FOUND && lastError != ERROR_FILE_NOT_FOUND) {
			throw std::system_error(static_cast<int>(lastError), std::system_category(), "GetUILanguageInfo");
		}
		return false;
	}
	return (attributes & MUI_LANGUAGE_INSTALLED) == MUI_LANGUAGE_INSTALLED;
}

}  // namespace m4t

void __asan_on_error() {  // NOLINT(readability-identifier-naming): Name required by ASAN.
	FAIL() << "Encountered an address sanitizer error";
}

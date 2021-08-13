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

#include "m4t/m4t.h"

#include <windows.h>

#include <string>
#include <system_error>

namespace m4t {

bool HasLocale(const std::string& locale) {
	std::wstring names(locale.cbegin(), locale.cend());
	names.push_back(L'\0');

	DWORD fallbackSize = 0;
	DWORD attributes = 0;
	if (!GetUILanguageInfo(MUI_LANGUAGE_NAME, names.c_str(), nullptr, &fallbackSize, &attributes)) {
		const DWORD lastError = GetLastError();
		if (lastError != ERROR_OBJECT_NOT_FOUND && lastError != ERROR_FILE_NOT_FOUND) {
			throw std::system_error(lastError, std::system_category(), "GetUILanguageInfo");
		}
		return false;
	}
	if ((attributes & MUI_LANGUAGE_INSTALLED) != MUI_LANGUAGE_INSTALLED) {
		return false;
	}
	return true;
}

namespace internal {

void LocaleSetter::SetUp(const std::string& locale) {
	ULONG bufferSize = 0;
	ASSERT_TRUE(GetThreadPreferredUILanguages(MUI_LANGUAGE_NAME | MUI_THREAD_LANGUAGES, &m_num, nullptr, &bufferSize));

	m_buffer = std::make_unique_for_overwrite<wchar_t[]>(bufferSize);
	ASSERT_TRUE(GetThreadPreferredUILanguages(MUI_LANGUAGE_NAME | MUI_THREAD_LANGUAGES, &m_num, m_buffer.get(), &bufferSize));

	std::wstring names(locale.cbegin(), locale.cend());
	names.push_back(L'\0');
	ULONG num = 1;
	ASSERT_TRUE(SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, names.c_str(), &num));
}

void LocaleSetter::TearDown() {
	ASSERT_TRUE(SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, m_buffer.get(), &m_num));
}

}  // namespace internal

}  // namespace m4t

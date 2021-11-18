/*
Copyright 2019 Michael Beckh

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
/// @brief Include the header file to allow compilation and analyzer checks.

#include "m4t/IStreamMock.h"

#include <objbase.h>

#include <cstring>
#include <new>
#include <string>

namespace m4t {

IStreamMock::IStreamMock() noexcept = default;
IStreamMock::~IStreamMock() noexcept = default;

HRESULT IStream_Stat::operator()(STATSTG* const arg, DWORD /* flags */) const {
	if (m_name) {
		const size_t cch = std::char_traits<wchar_t>::length(m_name) + 1;
		wchar_t* const pName = static_cast<wchar_t*>(CoTaskMemAlloc(cch * sizeof(wchar_t)));
		if (!pName) {
			[[unlikely]];
			throw std::bad_alloc();
		}
		std::memcpy(pName, m_name, cch * sizeof(wchar_t));
		arg->pwcsName = pName;
	} else {
		arg->pwcsName = nullptr;
	}

	return S_OK;
}

}  // namespace m4t

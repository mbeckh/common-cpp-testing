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

#pragma once

#include <gmock/gmock.h>

#include <windows.h>
#include <objidl.h>

namespace m4t {

/// @brief Mock class for `IStream`.
class IStreamMock : public IStream {
public:
	IStreamMock() noexcept;
	IStreamMock(const IStreamMock&) = delete;
	IStreamMock(IStreamMock&&) = delete;
	virtual ~IStreamMock() noexcept;

public:
	IStreamMock& operator=(const IStreamMock&) = delete;
	IStreamMock& operator=(IStreamMock&&) = delete;

public:
	// IUnknown
	MOCK_METHOD(HRESULT, QueryInterface, (REFIID riid, void** ppvObject), (Calltype(__stdcall), override));
	MOCK_METHOD(ULONG, AddRef, (), (Calltype(__stdcall), override));
	MOCK_METHOD(ULONG, Release, (), (Calltype(__stdcall), override));

public:
	// ISequentialStream
	MOCK_METHOD(HRESULT, Read, (void* pv, ULONG cb, ULONG* pcbRead), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, Write, (const void* pv, ULONG cb, ULONG* pcbWritten), (Calltype(__stdcall), override));

public:
	// IStream
	MOCK_METHOD(HRESULT, Seek, (LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, SetSize, (ULARGE_INTEGER libNewSize), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, CopyTo, (IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, Commit, (DWORD grfCommitFlags), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, Revert, (), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, LockRegion, (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, UnlockRegion, (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, Stat, (STATSTG * pstatstg, DWORD grfStatFlag), (Calltype(__stdcall), override));
	MOCK_METHOD(HRESULT, Clone, (IStream * *ppstm), (Calltype(__stdcall), override));
};

/// @brief Default action for `IStream::Stat`.
struct IStream_Stat {
	constexpr explicit IStream_Stat(const wchar_t* const name) noexcept
	    : m_name(name) {
		// empty
	}

	HRESULT operator()(STATSTG* arg, DWORD /* flags */) const;

private:
	const wchar_t* const m_name;
};

}  // namespace m4t

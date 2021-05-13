/*
Copyright 2019-2021 Michael Beckh

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

#include "m4t/MallocSpy.h"

#include <unknwn.h>

#include <mutex>
#include <shared_mutex>
#include <unordered_set>

namespace m4t {

//
// IUnknown
//

HRESULT MallocSpy::QueryInterface(REFIID riid, _COM_Outptr_ void** const ppObject) noexcept {
	if (!ppObject) {
		[[unlikely]];
		return E_INVALIDARG;
	}

	if (IsEqualIID(riid, IID_IMallocSpy) || IsEqualIID(riid, IID_IUnknown)) {
		[[likely]];
		*ppObject = this;
		static_cast<IMallocSpy*>(this)->AddRef();
		return S_OK;
	}
	*ppObject = nullptr;
	return E_NOINTERFACE;
}

ULONG MallocSpy::AddRef() noexcept {
	return InterlockedIncrement(&m_refCount);
}

ULONG MallocSpy::Release() noexcept {
	const ULONG refCount = InterlockedDecrement(&m_refCount);
	if (!refCount) {
		[[unlikely]];
		delete this;
	}
	return refCount;
}


//
// IMallocSpy
//

SIZE_T __stdcall MallocSpy::PreAlloc(_In_ const SIZE_T cbRequest) noexcept {
	return cbRequest;
}

void* __stdcall MallocSpy::PostAlloc(_In_ void* const pActual) noexcept {
	try {
		std::scoped_lock<decltype(m_mutex)> lock(m_mutex);
		m_allocated.insert(pActual);
	} catch (...) {
		// ignore
		[[unlikely]];
	}

	return pActual;
}

void* __stdcall MallocSpy::PreFree(_In_ void* const pRequest, _In_ const BOOL /* fSpyed */) noexcept {
	try {
		std::scoped_lock<decltype(m_mutex)> lock(m_mutex);
		m_allocated.erase(pRequest);
		m_deleted.insert(pRequest);
	} catch (...) {
		// ignore
		[[unlikely]];
	}

	return pRequest;
}

void __stdcall MallocSpy::PostFree(_In_ const BOOL /* fSpyed */) noexcept {
	// empty
}

SIZE_T __stdcall MallocSpy::PreRealloc(_In_ void* const pRequest, _In_ const SIZE_T cbRequest, _Outptr_ void** const ppNewRequest, _In_ BOOL /* fSpyed */) noexcept {
	try {
		std::scoped_lock<decltype(m_mutex)> lock(m_mutex);
		m_allocated.erase(pRequest);
		m_deleted.insert(pRequest);
	} catch (...) {
		// ignore
		[[unlikely]];
	}

	if (ppNewRequest) {
		[[likely]];
		*ppNewRequest = pRequest;
	}
	return cbRequest;
}

void* __stdcall MallocSpy::PostRealloc(_In_ void* const pActual, _In_ BOOL /* fSpyed */) noexcept {
	try {
		std::scoped_lock<decltype(m_mutex)> lock(m_mutex);
		m_allocated.insert(pActual);
	} catch (...) {
		// ignore
		[[unlikely]];
	}

	return pActual;
}

void* __stdcall MallocSpy::PreGetSize(_In_ void* const pRequest, _In_ BOOL /* fSpyed */) noexcept {
	return pRequest;
}

SIZE_T __stdcall MallocSpy::PostGetSize(_In_ const SIZE_T cbActual, _In_ BOOL /* fSpyed */) noexcept {
	return cbActual;
}

void* __stdcall MallocSpy::PreDidAlloc(_In_ void* const pRequest, _In_ BOOL /* fSpyed */) noexcept {
	return pRequest;
}

int __stdcall MallocSpy::PostDidAlloc(_In_ void* /* pRequest */, _In_ BOOL /* fSpyed */, _In_ const int fActual) noexcept {
	return fActual;
}

void __stdcall MallocSpy::PreHeapMinimize() noexcept {
	// empty
}

void __stdcall MallocSpy::PostHeapMinimize() noexcept {
	// empty
}

bool MallocSpy::IsAllocated(const void* const p) const {
	std::shared_lock<decltype(m_mutex)> lock(m_mutex);
	return m_allocated.contains(p);
}

bool MallocSpy::IsDeleted(const void* p) const {
	std::shared_lock<decltype(m_mutex)> lock(m_mutex);
	return m_deleted.contains(p);
}

std::size_t MallocSpy::GetAllocatedCount() const {
	std::shared_lock<decltype(m_mutex)> lock(m_mutex);
	return m_allocated.size();
}

std::size_t MallocSpy::GetDeletedCount() const {
	std::shared_lock<decltype(m_mutex)> lock(m_mutex);
	return m_deleted.size();
}

}  // namespace m4t

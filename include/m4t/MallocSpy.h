/*
Copyright 2019-2022 Michael Beckh

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

#include <windows.h>
#include <objidl.h>

#include <cstddef>
#include <set>
#include <shared_mutex>
#include <unordered_set>

namespace m4t {

/// @brief Implementation of `IMallocSpy` for use in testing.
class MallocSpy : public IMallocSpy {
public:
	MallocSpy() = default;
	MallocSpy(const MallocSpy&) = delete;
	MallocSpy(MallocSpy&&) = delete;
	// allow creation on the stack
	virtual ~MallocSpy() noexcept = default;

public:
	MallocSpy& operator=(const MallocSpy&) = delete;
	MallocSpy& operator=(MallocSpy&&) = delete;

public:  // IUnknown
	[[nodiscard]] HRESULT __stdcall QueryInterface(REFIID riid, _COM_Outptr_ void** ppObject) noexcept final;
	ULONG __stdcall AddRef() noexcept final;
	ULONG __stdcall Release() noexcept final;

public:  // IMallocSpy
	SIZE_T __stdcall PreAlloc(_In_ SIZE_T cbRequest) noexcept override;
	void* __stdcall PostAlloc(_In_ void* pActual) noexcept override;
	void* __stdcall PreFree(_In_ void* pRequest, _In_ BOOL fSpyed) noexcept override;
	void __stdcall PostFree(_In_ BOOL fSpyed) noexcept override;
	SIZE_T __stdcall PreRealloc(_In_ void* pRequest, _In_ SIZE_T cbRequest, _Outptr_ void** ppNewRequest, _In_ BOOL fSpyed) noexcept override;
	void* __stdcall PostRealloc(_In_ void* pActual, _In_ BOOL fSpyed) noexcept override;
	void* __stdcall PreGetSize(_In_ void* pRequest, _In_ BOOL fSpyed) noexcept override;
	SIZE_T __stdcall PostGetSize(_In_ SIZE_T cbActual, _In_ BOOL fSpyed) noexcept override;
	void* __stdcall PreDidAlloc(_In_ void* pRequest, _In_ BOOL fSpyed) noexcept override;
	int __stdcall PostDidAlloc(_In_ void* pRequest, _In_ BOOL fSpyed, _In_ int fActual) noexcept override;
	void __stdcall PreHeapMinimize() noexcept override;
	void __stdcall PostHeapMinimize() noexcept override;

public:  // MallocSpy
	bool IsAllocated(const void* p) const;
	bool IsDeleted(const void* p) const;
	std::size_t GetAllocatedCount() const;
	std::size_t GetDeletedCount() const;

private:
	volatile ULONG m_refCount = 1;  ///< @brief The COM reference count of this object.
	mutable std::shared_mutex m_mutex;
	std::unordered_set<const void*> m_allocated;  ///< @brief Currently allocated memory blocks.
	std::multiset<const void*> m_deleted;         ///< @brief All memory blocks that have ever been deleted.
};

}  // namespace m4t

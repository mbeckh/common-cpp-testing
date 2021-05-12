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

#include "m4t/MallocSpy.h"

#include "m4t/m4t.h"

#include <gtest/gtest.h>

#include <combaseapi.h>
#include <oaidl.h>
#include <unknwn.h>
#include <windows.h>

#include <cstddef>
#include <cstdlib>

namespace m4t::test {

TEST(MallocSpy, QueryInterface) {  // NOLINT(cert-err58-cpp, cppcoreguidelines-avoid-non-const-global-variables)
	MallocSpy* const pMallocSpy = new MallocSpy();

	EXPECT_EQ(E_INVALIDARG, pMallocSpy->QueryInterface(IID_IUnknown, nullptr));
	EXPECT_EQ(E_INVALIDARG, pMallocSpy->QueryInterface(IID_IDispatch, nullptr));

	IDispatch* pDispatch = kInvalidPtr<IDispatch>;
	EXPECT_EQ(E_NOINTERFACE, pMallocSpy->QueryInterface(IID_PPV_ARGS(&pDispatch)));
	EXPECT_NULL(pDispatch);

	IUnknown* pUnknown = kInvalidPtr<IUnknown>;
	EXPECT_HRESULT_SUCCEEDED(pMallocSpy->QueryInterface(IID_PPV_ARGS(&pUnknown)));
	ASSERT_EQ(pMallocSpy, pUnknown);
	EXPECT_EQ(1u, pUnknown->Release());

	EXPECT_EQ(0u, pMallocSpy->Release());
}

TEST(MallocSpy, AddRefRelease) {  // NOLINT(cert-err58-cpp, cppcoreguidelines-avoid-non-const-global-variables)
	thread_local bool deleted = false;

	class TrackingMallocSpy : public MallocSpy {
	public:
		static void* operator new(const std::size_t size) {
			return new std::byte[size];
		}
		static void operator delete(void* const ptr) noexcept {
			delete[] static_cast<std::byte*>(ptr);
			deleted = true;
		}
	};

	TrackingMallocSpy* const pMallocSpy = new TrackingMallocSpy();

	EXPECT_EQ(2u, pMallocSpy->AddRef());
	EXPECT_EQ(1u, pMallocSpy->Release());
	EXPECT_FALSE(deleted);

	EXPECT_EQ(0u, pMallocSpy->Release());

	EXPECT_TRUE(deleted);
}

TEST(MallocSpy, AllocFree) {  // NOLINT(cert-err58-cpp, cppcoreguidelines-avoid-non-const-global-variables)
	MallocSpy* const pMallocSpy = new MallocSpy();

	int* ptr = nullptr;

	EXPECT_EQ(0u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(0u, pMallocSpy->GetDeletedCount());

	EXPECT_EQ(sizeof(*ptr), pMallocSpy->PreAlloc(sizeof(*ptr)));

	EXPECT_EQ(0u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(0u, pMallocSpy->GetDeletedCount());

	ptr = new int;

	EXPECT_EQ(ptr, pMallocSpy->PostAlloc(ptr));

	EXPECT_TRUE(pMallocSpy->IsAllocated(ptr));
	EXPECT_FALSE(pMallocSpy->IsDeleted(ptr));
	EXPECT_EQ(1u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(0u, pMallocSpy->GetDeletedCount());

	EXPECT_EQ(ptr, pMallocSpy->PreFree(ptr, TRUE));

	EXPECT_FALSE(pMallocSpy->IsAllocated(ptr));
	EXPECT_TRUE(pMallocSpy->IsDeleted(ptr));
	EXPECT_EQ(0u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(1u, pMallocSpy->GetDeletedCount());

	int* ptrValue = ptr;
	delete ptr;

	pMallocSpy->PostFree(TRUE);

	EXPECT_FALSE(pMallocSpy->IsAllocated(ptrValue));  // NOLINT(clang-analyzer-cplusplus.NewDelete)
	EXPECT_TRUE(pMallocSpy->IsDeleted(ptrValue));
	EXPECT_EQ(0u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(1u, pMallocSpy->GetDeletedCount());

	pMallocSpy->Release();
}

TEST(MallocSpy, Realloc) {  // NOLINT(cert-err58-cpp, cppcoreguidelines-avoid-non-const-global-variables)
	MallocSpy* const pMallocSpy = new MallocSpy();

	void* ptr = nullptr;
	constexpr std::size_t kSize = 10;

	EXPECT_EQ(kSize, pMallocSpy->PreAlloc(kSize));
	ptr = std::malloc(kSize);  // NOLINT(cppcoreguidelines-no-malloc)
	EXPECT_EQ(ptr, pMallocSpy->PostAlloc(ptr));

	EXPECT_TRUE(pMallocSpy->IsAllocated(ptr));
	EXPECT_FALSE(pMallocSpy->IsDeleted(ptr));
	EXPECT_EQ(1u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(0u, pMallocSpy->GetDeletedCount());

	void* ptrNew = nullptr;
	EXPECT_EQ(kSize * 2, pMallocSpy->PreRealloc(ptr, kSize * 2, &ptrNew, TRUE));
	EXPECT_EQ(ptr, ptrNew);

	EXPECT_FALSE(pMallocSpy->IsAllocated(ptr));
	EXPECT_TRUE(pMallocSpy->IsDeleted(ptr));
	EXPECT_EQ(0u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(1u, pMallocSpy->GetDeletedCount());

	ptrNew = std::realloc(ptrNew, kSize * 2);  // NOLINT(cppcoreguidelines-no-malloc)

	EXPECT_EQ(ptrNew, pMallocSpy->PostRealloc(ptrNew, TRUE));

	EXPECT_TRUE(pMallocSpy->IsAllocated(ptrNew));
	EXPECT_FALSE(pMallocSpy->IsDeleted(ptrNew));
	EXPECT_EQ(1u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(1u, pMallocSpy->GetDeletedCount());

	EXPECT_EQ(ptrNew, pMallocSpy->PreFree(ptrNew, TRUE));

	EXPECT_FALSE(pMallocSpy->IsAllocated(ptrNew));
	EXPECT_TRUE(pMallocSpy->IsDeleted(ptrNew));
	EXPECT_EQ(0u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(2u, pMallocSpy->GetDeletedCount());

	void* ptrValue = ptrNew;
	std::free(ptrNew);  // NOLINT(cppcoreguidelines-no-malloc)

	pMallocSpy->PostFree(TRUE);

	EXPECT_FALSE(pMallocSpy->IsAllocated(ptrValue));  // NOLINT(clang-analyzer-unix.Malloc)
	EXPECT_TRUE(pMallocSpy->IsDeleted(ptrValue));
	EXPECT_EQ(0u, pMallocSpy->GetAllocatedCount());
	EXPECT_EQ(2u, pMallocSpy->GetDeletedCount());

	pMallocSpy->Release();
}

TEST(MallocSpy, GetSizeDidAllocHeapMinimize) {  // NOLINT(cert-err58-cpp, cppcoreguidelines-avoid-non-const-global-variables)
	MallocSpy* const pMallocSpy = new MallocSpy();

	int* ptr = new int;

	EXPECT_EQ(ptr, pMallocSpy->PreGetSize(ptr, TRUE));
	EXPECT_EQ(sizeof(*ptr), pMallocSpy->PostGetSize(sizeof(*ptr), TRUE));

	EXPECT_EQ(ptr, pMallocSpy->PreDidAlloc(ptr, TRUE));
	EXPECT_EQ(-1, pMallocSpy->PostDidAlloc(ptr, TRUE, -1));

	pMallocSpy->PreHeapMinimize();
	pMallocSpy->PostHeapMinimize();

	delete ptr;

	pMallocSpy->Release();
}

}  // namespace m4t::test

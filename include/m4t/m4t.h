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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <windows.h>
#include <objidl.h>
#include <propidl.h>
#include <propvarutil.h>
#include <unknwn.h>
#include <wtypes.h>

#include <cstddef>
#include <memory>
#include <ostream>
#include <regex>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

//
// Additional checks
//

/// @brief Generates a failure if @p arg_ is not `nullptr`.
/// @param arg_ The argument to check.
#define ASSERT_NULL(arg_) ASSERT_EQ(nullptr, (arg_))

/// @brief Generates a failure if @p arg_ is `nullptr`.
/// @param arg_ The argument to check.
#define ASSERT_NOT_NULL(arg_) ASSERT_NE(nullptr, (arg_))

/// @brief Generates a failure if @p arg_ is not `nullptr`.
/// @param arg_ The argument to check.
#define EXPECT_NULL(arg_) EXPECT_EQ(nullptr, (arg_))

/// @brief Generates a failure if @p arg_ is `nullptr`.
/// @param arg_ The argument to check.
#define EXPECT_NOT_NULL(arg_) EXPECT_NE(nullptr, (arg_))

#if defined(__SANITIZE_ADDRESS__)
extern "C" {
int __asan_address_is_poisoned(void const volatile* addr);
}

/// @brief Generates a failure if @p p_ is not uninitialized memory.
/// @param p_ The pointer to check. It MUST point to at least 4 valid bytes of memory.
#define EXPECT_UNINITIALIZED(p_) __pragma(warning(suppress : 6001)) EXPECT_EQ(0xCDCDCDCD, *std::bit_cast<std::uint32_t*>((p_)))

/// @brief Generates a failure if @p p_ is not deleted memory.
/// @warning This macro requires ASan AddressSanitizer, else it is a simple not-null check.
/// @param p_ The pointer to check.
#define EXPECT_DELETED(p_) EXPECT_EQ(1, __asan_address_is_poisoned((p_)))

#elif defined(__clang_analyzer__) || (defined(NDEBUG) && NDEBUG) || !defined(_DEBUG) || !_DEBUG

/// @brief Generates a failure if @p p_ is not uninitialized memory.
/// @param p_ The pointer to check. It MUST point to at least 4 valid bytes of memory.
#define EXPECT_UNINITIALIZED(p_) EXPECT_NOT_NULL(p_)

/// @brief Generates a failure if @p p_ is not deleted memory.
/// @warning This macro requires ASan AddressSanitizer, else it is a simple not-null check.
/// @param p_ The pointer to check.
#define EXPECT_DELETED(p_) EXPECT_NOT_NULL(p_)

#else

/// @brief Convert argument to string.
/// @param arg_ The value.
#define M4T_STRINGIZE(arg_) #arg_

/// @brief Convert second argument to string.
/// @param macro_ The stringize macro.
/// @param arg_ The value.
/// @see https://stackoverflow.com/a/5966882
#define M4T_MAKE_STRING(macro_, value_) macro_(value_)

/// @brief Generates a failure if @p p_ is not uninitialized memory.
/// @param p_ The pointer to check. It MUST point to at least 4 valid bytes of memory.
#define EXPECT_UNINITIALIZED(p_) __pragma(warning(suppress : 6001)) EXPECT_EQ(0xCDCDCDCD, *std::bit_cast<std::uint32_t*>((p_)))

/// @brief Generates a failure if @p p_ is not deleted memory.
/// @warning This macro requires ASan AddressSanitizer, else it is a no-op.
/// @param p_ The pointer to check.
#define EXPECT_DELETED(p_) __pragma(message(__FILE__ "(" M4T_MAKE_STRING(M4T_STRINGIZE, __LINE__) "): Deleted check requires ASAN"))
#endif

//
// Mock helpers
//

/// @brief Declare a COM mock object.
/// @param name_ The name of the mock object.
/// @param type_ The type of the mock object.
#define COM_MOCK_DECLARE(name_, type_) \
	type_ name_;                       \
	ULONG name_##RefCount_ = 1

/// @brief Prepare a COM mock object for use.
/// @details Sets up basic calls for `AddRef`, `Release` and `QueryInterface`.
/// Provide all COM interfaces as additional arguments.
/// @brief @param name_ The name of the mock object.
#define COM_MOCK_SETUP(name_, ...) \
	m4t::SetupComMock<decltype(name_), __VA_ARGS__>((name_), name_##RefCount_)

/// @brief Verify that the reference count of a COM mock is 1.
/// @param name_ The name of the mock object.
#define COM_MOCK_VERIFY(name_) \
	EXPECT_EQ(1, name_##RefCount_) << "Reference count of " #name_

/// @brief Verify that the reference count of a COM mock has a particular value.
/// @param count_ The expected reference count.
/// @param name_ The name of the mock object.
#define COM_MOCK_EXPECT_REFCOUNT(count_, name_) \
	EXPECT_EQ((count_), name_##RefCount_)


namespace m4t {

namespace t = testing;

namespace internal {
inline constexpr int kInvalid = 0;  ///< @brief A variable whose address serves as a marker for invalid pointer values.
}  // namespace internal

/// @brief A value to be used for marking pointer values as invalid.
/// @details Not const or constexpr to allow assignment to non-const pointers.
template <typename T>
inline T* const kInvalidPtr = reinterpret_cast<T*>(const_cast<int*>(&internal::kInvalid));  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-pro-type-const-cast)

/// @brief A matcher that returns `true` if @p bits are set.
/// @param bits The bit pattern to check.
#pragma warning(suppress : 4100)
MATCHER_P(BitsSet, bits, "") {
	using ArgTypeUnsigned = std::make_unsigned_t<std::remove_cvref_t<arg_type>>;
	using BitsTypeUnsigned = std::make_unsigned_t<std::remove_cvref_t<bits_type>>;
	return (static_cast<ArgTypeUnsigned>(arg) & static_cast<BitsTypeUnsigned>(bits)) == static_cast<BitsTypeUnsigned>(bits);
}

/// @brief Create a matcher using `std::regex` instead of the regex library of googletest.
/// @details The argument MUST match the whole regex.
/// @param pattern The regex or regex pattern to use.
#pragma warning(suppress : 4100)
MATCHER_P(MatchesRegex, pattern, "") {
	if constexpr (std::is_same_v<pattern_type, std::regex> || std::is_same_v<pattern_type, std::wregex>) {
		return std::regex_match(arg, pattern);
	} else if constexpr (std::is_constructible_v<std::regex, pattern_type>) {
		return std::regex_match(arg, std::regex(pattern));
	} else if constexpr (std::is_constructible_v<std::wregex, pattern_type>) {
		return std::regex_match(arg, std::wregex(pattern));
	} else {
		static_assert(sizeof(pattern_type) == 0);
	}
}

/// @brief Create a matcher using `std::regex` instead of the regex library of googletest.
/// @details The argument MUST contain the regex, but not match as a whole. Use anchoring if this is required.
/// @param pattern The regex or regex pattern to use.
#pragma warning(suppress : 4100)
MATCHER_P(ContainsRegex, pattern, "") {
	if constexpr (std::is_same_v<pattern_type, std::regex> || std::is_same_v<pattern_type, std::wregex>) {
		return std::regex_search(arg, pattern);
	} else if constexpr (std::is_constructible_v<std::regex, pattern_type>) {
		return std::regex_search(arg, std::regex(pattern));
	} else if constexpr (std::is_constructible_v<std::wregex, pattern_type>) {
		return std::regex_search(arg, std::wregex(pattern));
	} else {
		static_assert(sizeof(pattern_type) == 0);
	}
}

namespace internal {

template <typename AsType>
class PointerAsMatcher {
public:
	using is_gtest_matcher = void;

	explicit PointerAsMatcher(t::Matcher<const AsType*> matcher)
	    : m_matcher(std::move(matcher)) {
	}

	template <typename Pointer>
	bool MatchAndExplain(Pointer pointer, t::MatchResultListener* listener) const {
		const AsType* const ptr = reinterpret_cast<const AsType*>(std::to_address(pointer));
		if (!ptr) {
			return false;
		}

		*listener << "which is a pointer to ";
		return MatchPrintAndExplain(ptr, m_matcher, listener);
	}

	void DescribeTo(std::ostream* os) const {
		*os << "is a pointer of type " << typeid(AsType).name() << "* that ";
		m_matcher.DescribeTo(os);
	}

	void DescribeNegationTo(std::ostream* os) const {
		*os << "is a pointer of type " << typeid(AsType).name() << "* that ";
		m_matcher.DescribeTo(os);
	}

private:
	const t::Matcher<const AsType*> m_matcher;
};

template <typename AsType>
class PointeeAsMatcher {
public:
	using is_gtest_matcher = void;

	explicit PointeeAsMatcher(t::Matcher<const AsType&> matcher)
	    : m_matcher(std::move(matcher)) {
	}

	template <typename Pointer>
	bool MatchAndExplain(Pointer pointer, t::MatchResultListener* listener) const {
		const AsType* const ptr = reinterpret_cast<const AsType*>(std::to_address(pointer));
		if (!ptr) {
			return false;
		}

		*listener << "which points to ";
		return MatchPrintAndExplain(*ptr, m_matcher, listener);
	}

	void DescribeTo(std::ostream* os) const {
		*os << "points to a value of type " << typeid(AsType).name() << " that ";
		m_matcher.DescribeTo(os);
	}

	void DescribeNegationTo(std::ostream* os) const {
		*os << "does not point to a value of type " << typeid(AsType).name() << " that ";
		m_matcher.DescribeTo(os);
	}

private:
	const t::Matcher<const AsType&> m_matcher;
};

}  // namespace internal

template <typename AsType>
inline internal::PointerAsMatcher<AsType> PointerAs(const t::Matcher<const AsType*>& matcher) {
	return internal::PointerAsMatcher<AsType>(matcher);
}

template <typename AsType>
inline internal::PointeeAsMatcher<AsType> PointeeAs(const t::Matcher<const AsType&>& matcher) {
	return internal::PointeeAsMatcher<AsType>(matcher);
}

namespace internal {

/// @brief A helper class for `WithLocale`.
class LocaleSetter {
public:
	explicit LocaleSetter(const std::string& locale) {
		SetUp(locale);
	}
	LocaleSetter(const LocaleSetter&) = delete;
	LocaleSetter(LocaleSetter&&) = delete;
	~LocaleSetter() {
		TearDown();
	}

public:
	LocaleSetter& operator=(const LocaleSetter&) = delete;
	LocaleSetter& operator=(LocaleSetter&&) = delete;

private:
	void SetUp(const std::string& locale);
	void TearDown();

private:
	ULONG m_num = 0;
	std::unique_ptr<wchar_t[]> m_buffer;
};

}  // namespace internal

/// @brief Check if a locale is installed.
bool HasLocale(const std::string& locale);

/// @brief Run some code in a thread using a user-defined locale.
/// @tparam L The type of the closure.
/// @param locale The name of the locale.
/// @param lambda The code as a closure with no arguments.
/// @return The result of calling @p lambda.
template <typename L>
inline auto WithLocale(const std::string& locale, L&& lambda) {
	const internal::LocaleSetter setter(locale);
	return lambda();
}

/// @brief Action for setting Windows error code.
/// @param lastError The value for SetLastError().
constexpr auto SetLastError(const DWORD lastError) noexcept {
	return [lastError]() noexcept -> void {
		::SetLastError(lastError);
	};
}

/// @brief Action for setting Windows error code and returning a value.
/// @tparam T The type of the result argument.
/// @param lastError The value for SetLastError().
/// @param value The return value of the action.
template <typename T>
constexpr auto SetLastErrorAndReturn(const DWORD lastError, T&& value) noexcept {
	return [lastError, value = std::forward<T>(value)]() noexcept -> T {
		::SetLastError(lastError);
		return value;
	};
}

/// @brief Action for mocking `IUnknown::AddRef`.
/// @param refCount A reference to the variable holding the COM reference count.
constexpr auto AddRef(ULONG& refCount) noexcept {
	return [&refCount]() constexpr noexcept->ULONG {
		return ++refCount;
	};
}

/// @brief Action for mocking `IUnknown::Release`.
/// @param refCount A reference to the variable holding the COM reference count.
constexpr auto Release(ULONG& refCount) noexcept {
	return [&refCount]() constexpr noexcept->ULONG {
		return --refCount;
	};
}

/// @brief Action for mocking `IUnknown::QueryInterface`.
/// @details Always returns `S_OK`.
/// @param pObject The object to return as a result.
template <std::derived_from<IUnknown> T>
constexpr auto QueryInterface(T* const pObject) noexcept {
	return [pObject](REFIID, void** ppv) noexcept -> HRESULT {
		*ppv = pObject;
		static_cast<IUnknown*>(pObject)->AddRef();
		return S_OK;
	};
};

/// @brief Action for mocking `IUnknown::QueryInterface` returning a failure.
/// @details Always returns `E_NOINTERFACE` and sets the pointer to `nullptr`.
constexpr auto QueryInterfaceFail() noexcept {
	return [](REFIID, void** ppv) constexpr noexcept->HRESULT {
		*ppv = nullptr;
		return E_NOINTERFACE;
	};
}

/// @brief Helper function to setup a mock object.
/// @tparam M The class of the mock object.
/// @tparam T The COM interfaces implemented by the mock object.
/// @param mock The mock object.
/// @param refCount The variable holding the mock reference count.
template <class M, class... T>
inline void SetupComMock(M& mock, ULONG& refCount) {
	// allow removing expectations without removing default behavior
	ON_CALL(mock, AddRef)
	    .WillByDefault(m4t::AddRef(refCount));
	ON_CALL(mock, Release)
	    .WillByDefault(m4t::Release(refCount));
	ON_CALL(mock, QueryInterface)
	    .WillByDefault(m4t::QueryInterfaceFail());
	ON_CALL(mock, QueryInterface(t::AnyOf(__uuidof(IUnknown), __uuidof(T)...), t::_))
	    .WillByDefault(m4t::QueryInterface(&mock));

	EXPECT_CALL(mock, AddRef).Times(t::AnyNumber());
	EXPECT_CALL(mock, Release).Times(t::AnyNumber());
	EXPECT_CALL(mock, QueryInterface).Times(t::AnyNumber());
	EXPECT_CALL(mock, QueryInterface(t::AnyOf(__uuidof(IUnknown), __uuidof(T)...), t::_)).Times(t::AnyNumber());
}

/// @brief Action for returning a COM object as an output argument.
/// @details Shortcut for `DoAll(SetArgPointee<idx>(&m_object), IgnoreResult(AddRef(&m_objectRefCount)))`.
/// Usage: `SetComObject<1>(&m_object)`. The output argument MUST NOT be null.
/// @tparam kIndex 0-based index of the argument.
/// @tparam T The type of the COM object.
/// @param pObject A pointer to a COM object.
template <std::size_t kIndex, std::derived_from<IUnknown> T>
constexpr auto SetComObject(T* const pObject) {
	return [pObject](auto&&... args) constexpr noexcept->void {
		*std::get<kIndex>(std::make_tuple<decltype(args)...>(std::forward<decltype(args)>(args)...)) = pObject;
		pObject->AddRef();
	};
}

/// @brief Action for setting a pointer to a `PROPVARIANT` to a `VARIANT_BOOL` value.
/// @details Usage: `SetPropVariantToBool<1>(VARIANT_TRUE)`. The `PROPVARIANT` MUST NOT be null.
/// @tparam kIndex 0-based index of the argument.
/// @param variantBool A `VARIANT_BOOL`.
template <std::size_t kIndex>
constexpr auto SetPropVariantToBool(const VARIANT_BOOL variantBool) noexcept {
	return [variantBool](auto&&... args) constexpr noexcept->void {
		PROPVARIANT* const ppv = std::get<kIndex>(std::make_tuple<decltype(args)...>(std::forward<decltype(args)>(args)...));
		ppv->boolVal = variantBool;
		ppv->vt = VT_BOOL;
	};
}

/// @brief Action for setting a pointer to a `PROPVARIANT` to a `BSTR` value.
/// @details Usage: `SetPropVariantToBool<1>("value")`. The `PROPVARIANT` MUST NOT be null.
/// @tparam kIndex 0-based index of the argument.
/// @param wsz A wide character string.
template <std::size_t kIndex>
constexpr auto SetPropVariantToBSTR(const wchar_t* const wsz) noexcept {
	return [wsz](auto&&... args) -> void {
		PROPVARIANT* const ppv = std::get<kIndex>(std::make_tuple<decltype(args)...>(std::forward<decltype(args)>(args)...));
		PROPVARIANT pv;
		HRESULT hr = InitPropVariantFromString(wsz, &pv);
		if (FAILED(hr)) {
			[[unlikely]];
			throw std::system_error(hr, std::system_category(), "InitPropVariantFromString");
		}
		hr = PropVariantChangeType(ppv, pv, 0, VARENUM::VT_BSTR);
		if (FAILED(hr)) {
			[[unlikely]];
			throw std::system_error(hr, std::system_category(), "PropVariantChangeType");
		}
	};
}

/// @brief Action for setting a pointer to a `PROPVARIANT` to `VT_EMPTY`.
/// @details Usage: `SetPropVariantToEmpty<1>()`. The `PROPVARIANT` MUST NOT be null.
/// @tparam kIndex 0-based index of the argument.
template <std::size_t kIndex>
constexpr auto SetPropVariantToEmpty() noexcept {
	return [](auto&&... args) -> void {
		PROPVARIANT* const ppv = std::get<kIndex>(std::make_tuple<decltype(args)...>(std::forward<decltype(args)>(args)...));
		const HRESULT hr = PropVariantClear(ppv);
		if (FAILED(hr)) {
			[[unlikely]];
			throw std::system_error(hr, std::system_category(), "PropVariantClear");
		}
	};
}

/// @brief Action for setting pointer to a `PROPVARIANT` to an `IStream` value.
/// @details Usage: `SetPropVariantToStream<1>(pStream)`.  The `PROPVARIANT` MUST NOT be null.
/// @tparam kIndex 0-based index of the argument.
/// @param pStream An object of type `IStream`.
template <std::size_t kIndex>
constexpr auto SetPropVariantToStream(IStream* const pStream) noexcept {
	return [pStream](auto&&... args) noexcept -> void {
		PROPVARIANT* const ppv = std::get<kIndex>(std::make_tuple<decltype(args)...>(std::forward<decltype(args)>(args)...));
		pStream->AddRef();
		ppv->pStream = pStream;
		ppv->vt = VT_STREAM;
	};
}

/// @brief Action for setting pointer to a `PROPVARIANT` to a `ULONG` value.
/// @details Usage: `SetPropVariantToUInt32<1>(value)`. The `PROPVARIANT` MUST NOT be null.
/// @tparam kIndex 0-based index of the argument.
/// @param value An unsigned integer value.
template <std::size_t kIndex>
constexpr auto SetPropVariantToUInt32(const ULONG value) noexcept {
	return [value](auto&&... args) constexpr noexcept->void {
		PROPVARIANT* const ppv = std::get<kIndex>(std::make_tuple<decltype(args)...>(std::forward<decltype(args)>(args)...));
		ppv->ulVal = value;
		ppv->vt = VARENUM::VT_UI4;
	};
}


/// @brief Mark an object as initialized for static analysis.
/// @details Out function is considered re-initialization by clang static analysis.
/// @tparam T The type of the object.
template <typename T>
constexpr void EnableMovedFromCheck(_Out_ T& /* obj */) noexcept {
	// empty
}

}  // namespace m4t

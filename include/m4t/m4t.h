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
#include <gtest/gtest.h>

#include <windows.h>
#include <propidl.h>
#include <propvarutil.h>
#include <unknwn.h>
#include <wtypes.h>

#include <regex>
#include <system_error>
#include <tuple>
#include <type_traits>

#ifdef M4T_GOOGLETEST_SHORT_NAMESPACE
namespace M4T_GOOGLETEST_SHORT_NAMESPACE = testing;
#else
namespace t = testing;
#endif

//
// Additional checks
//

/// @brief Generates a failure if @p arg_ is not `nullptr`.
/// @param arg_ The argument to check.
#define ASSERT_NULL(arg_) ASSERT_EQ(nullptr, (arg_))  // NOLINT(cppcoreguidelines-macro-usage)

/// @brief Generates a failure if @p arg_ is `nullptr`.
/// @param arg_ The argument to check.
#define ASSERT_NOT_NULL(arg_) ASSERT_NE(nullptr, (arg_))  // NOLINT(cppcoreguidelines-macro-usage)

/// @brief Generates a failure if @p arg_ is not `nullptr`.
/// @param arg_ The argument to check.
#define EXPECT_NULL(arg_) EXPECT_EQ(nullptr, (arg_))  // NOLINT(cppcoreguidelines-macro-usage)

/// @brief Generates a failure if @p arg_ is `nullptr`.
/// @param arg_ The argument to check.
#define EXPECT_NOT_NULL(arg_) EXPECT_NE(nullptr, (arg_))  // NOLINT(cppcoreguidelines-macro-usage)

#ifdef __SANITIZE_ADDRESS__
extern "C" {
int __asan_address_is_poisoned(void const volatile* addr);
}

/// @brief Generates a failure if @p p_ is not uninitialized memory.
/// @param p_ The pointer to check. It MUST point to at least 4 valid bytes of memory.
#define EXPECT_UNINITIALIZED(p_) __pragma(warning(suppress : 6001)) EXPECT_EQ(0xCDCDCDCD, *std::bit_cast<std::uint32_t*>((p_)))  // NOLINT(cppcoreguidelines-macro-usage)

/// @brief Generates a failure if @p p_ is not deleted memory.
/// @warning This macro requires ASan AddressSanitizer, else it is a no-op.
/// @param p_ The pointer to check.
#define EXPECT_DELETED(p_) EXPECT_EQ(1, __asan_address_is_poisoned((p_)))  // NOLINT(cppcoreguidelines-macro-usage)
#else

/// @brief Convert argument to string.
/// @param arg_ The value.
#define M4T_STRINGIZE(arg_) #arg_                                                                                                     // NOLINT(cppcoreguidelines-macro-usage)

/// @brief Convert second argument to string.
/// @param macro_ The stringize macro.
/// @param arg_ The value.
/// @see https://stackoverflow.com/a/5966882
#define M4T_MAKE_STRING(macro_, value_) macro_(value_)                                                                                // NOLINT(cppcoreguidelines-macro-usage)

/// @brief Generates a failure if @p p_ is not uninitialized memory.
/// @param p_ The pointer to check. It MUST point to at least 4 valid bytes of memory.
#define EXPECT_UNINITIALIZED(p_) __pragma(warning(suppress : 6001)) EXPECT_EQ(0xCDCDCDCD, *std::bit_cast<std::uint32_t*>((p_)))       // NOLINT(cppcoreguidelines-macro-usage)

/// @brief Generates a failure if @p p_ is not deleted memory.
/// @warning This macro requires ASan AddressSanitizer, else it is a no-op.
/// @param p_ The pointer to check.
#define EXPECT_DELETED(p_) __pragma(message(__FILE__ "(" M4T_MAKE_STRING(M4T_STRINGIZE, __LINE__) "): Deleted check requires ASAN"))  // NOLINT(cppcoreguidelines-macro-usage)
#endif

//
// Mock helpers
//

/// @brief Declare a COM mock object.
/// @param name_ The name of the mock object.
/// @param type_ The type of the mock object.
#define COM_MOCK_DECLARE(name_, type_) /* NOLINT(cppcoreguidelines-macro-usage) */ \
	type_ name_;                                                                   \
	ULONG name_##RefCount_ = 1

/// @brief Prepare a COM mock object for use.
/// @details Sets up basic calls for `AddRef`, `Release` and `QueryInterface`.
/// Provide all COM interfaces as additional arguments.
/// @brief @param name_ The name of the mock object.
#define COM_MOCK_SETUP(name_, ...) /* NOLINT(cppcoreguidelines-macro-usage) */ \
	m4t::SetupComMock<decltype(name_), __VA_ARGS__>((name_), name_##RefCount_)

/// @brief Verify that the reference count of a COM mock is 1.
/// @param name_ The name of the mock object.
#define COM_MOCK_VERIFY(name_) /* NOLINT(cppcoreguidelines-macro-usage) */ \
	EXPECT_EQ(1uL, name_##RefCount_) << "Reference count of " #name_

/// @brief Verify that the reference count of a COM mock has a particular value.
/// @param count_ The expected reference count.
/// @param name_ The name of the mock object.
#define COM_MOCK_EXPECT_REFCOUNT(count_, name_) /* NOLINT(cppcoreguidelines-macro-usage) */ \
	EXPECT_EQ((count_), name_##RefCount_)


namespace m4t {

namespace t = testing;

namespace internal {
constexpr int kInvalid = 0;  ///< @brief A variable whose address serves as a marker for invalid pointer values.
}  // namespace internal

/// @brief A value to be used for marking pointer values as invalid.
template <typename T>
T* kInvalidPtr = reinterpret_cast<T*>(const_cast<int*>(&internal::kInvalid));  // NOLINT(cppcoreguidelines-pro-type-const-cast, cppcoreguidelines-avoid-non-const-global-variables, readability-identifier-naming)

/// @brief A matcher that returns `true` if @p bits are set.
/// @param bits The bit pattern to check.
#pragma warning(suppress : 4100)
MATCHER_P(BitsSet, bits, "") {
	return (arg & bits) == bits;
}

/// @brief Create a matcher using `std::regex` instead of the regex library of googletest.
/// @param pattern The regex or regex pattern to use.
#pragma warning(suppress : 4100)
MATCHER_P(MatchesRegex, pattern, "") {  // NOLINT(readability-redundant-string-init, misc-non-private-member-variables-in-classes)
	if constexpr (std::is_same_v<pattern_type, std::regex> || std::is_same_v<pattern_type, std::wregex>) {
		return std::regex_match(arg, pattern);
	} else if constexpr (std::is_constructible_v<std::regex, pattern_type>) {
		return std::regex_match(arg, std::regex(pattern));
	} else if constexpr (std::is_constructible_v<std::wregex, pattern_type>) {
		return std::regex_match(arg, std::wregex(pattern));
	} else {
#ifndef __clang_analyzer__
		static_assert(false);
#endif
	}
}


namespace internal {

/// @brief A helper class for `WithLocale`.
class LocaleSetter {
public:
	explicit LocaleSetter(const std::string& locale) {
		SetUp(locale);
	}
	~LocaleSetter() {
		TearDown();
	}

private:
	void SetUp(const std::string& locale);
	void TearDown();

private:
	ULONG m_num;
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
auto WithLocale(const std::string& locale, L&& lambda) {
	const internal::LocaleSetter setter(locale);
	return lambda();
}


/// @brief Action for mocking `IUnknown::AddRef`.
/// @param pRefCount A pointer to the variable holding the COM reference count.
ACTION_P(AddRef, pRefCount) {  // NOLINT(cppcoreguidelines-special-member-functions, misc-non-private-member-variables-in-classes)
	static_assert(std::is_same_v<ULONG*, pRefCount_type>);
	return ++*pRefCount;
}

/// @brief Action for mocking `IUnknown::Release`.
/// @param pRefCount A pointer to the variable holding the COM reference count.
ACTION_P(Release, pRefCount) {  // NOLINT(cppcoreguidelines-special-member-functions, misc-non-private-member-variables-in-classes)
	static_assert(std::is_same_v<ULONG*, pRefCount_type>);
	return --*pRefCount;
}

/// @brief Action for mocking `IUnknown::QueryInterface`.
/// @param pObject The object to return as a result.
ACTION_P(QueryInterface, pObject) {  // NOLINT(cppcoreguidelines-special-member-functions, misc-non-private-member-variables-in-classes)
	static_assert(std::is_same_v<REFIID, arg0_type>);
	static_assert(std::is_same_v<void**, arg1_type>);
	static_assert(std::is_base_of_v<IUnknown, std::remove_pointer_t<pObject_type>>);

	*arg1 = pObject;
	static_cast<IUnknown*>(pObject)->AddRef();
	return S_OK;
}

/// @brief Action for mocking `IUnknown::QueryInterface` returning a failure.
ACTION(QueryInterfaceFail) {  // NOLINT(cppcoreguidelines-special-member-functions)
	static_assert(std::is_same_v<REFIID, arg0_type>);
	static_assert(std::is_same_v<void**, arg1_type>);

	*arg1 = nullptr;
	return E_NOINTERFACE;
}

/// @brief Helper function to setup a mock object.
/// @tparam M The class of the mock object.
/// @tparam T The COM interfaces implemented by the mock object.
/// @param mock The mock object.
/// @param refCount The variable holding the mock reference count.
template <class M, class... T>
void SetupComMock(M& mock, ULONG& refCount) {
	// allow removing expectations without removing default behavior
	ON_CALL(mock, AddRef())
		.WillByDefault(m4t::AddRef(&refCount));
	ON_CALL(mock, Release())
		.WillByDefault(m4t::Release(&refCount));
	ON_CALL(mock, QueryInterface(t::_, t::_))
		.WillByDefault(m4t::QueryInterfaceFail());
	ON_CALL(mock, QueryInterface(t::AnyOf(__uuidof(IUnknown), __uuidof(T)...), t::_))
		.WillByDefault(m4t::QueryInterface(&mock));

	EXPECT_CALL(mock, AddRef()).Times(t::AtLeast(0));
	EXPECT_CALL(mock, Release()).Times(t::AtLeast(0));
	EXPECT_CALL(mock, QueryInterface(t::_, t::_)).Times(t::AtLeast(0));
	EXPECT_CALL(mock, QueryInterface(t::AnyOf(__uuidof(IUnknown), __uuidof(T)...), t::_)).Times(t::AtLeast(0));
}

/// @brief Action for returning a COM object as an output argument.
/// @details Shortcut for `DoAll(SetArgPointee<idx>(&m_object), IgnoreResult(AddRef(&m_objectRefCount)))`.
/// Usage: `SetComObjectAndReturnOk<1>(&m_object)`. The output argument MUST NOT be null.
/// @tparam idx 0-based index of the argument.
/// @param pObject A pointer to a COM object.
ACTION_TEMPLATE(SetComObject, HAS_1_TEMPLATE_PARAMS(int, idx), AND_1_VALUE_PARAMS(pObject)) {  // NOLINT(cppcoreguidelines-special-member-functions, misc-non-private-member-variables-in-classes)
	using idx_type = typename std::tuple_element<idx, args_type>::type;                        // NOLINT(readability-identifier-naming)
	static_assert(std::is_base_of_v<IUnknown, std::remove_pointer_t<std::remove_pointer_t<idx_type>>>);
	static_assert(std::is_base_of_v<IUnknown, std::remove_pointer_t<pObject_type>>);

	*t::get<idx>(args) = pObject;
	pObject->AddRef();
}

/// @brief Action for setting a pointer to a `PROPVARIANT` to a `VARIANT_BOOL` value.
/// @details Usage: `SetPropVariantToBool<1>(VARIANT_TRUE)`. The `PROPVARIANT` MUST NOT be null.
/// @tparam idx 0-based index of the argument.
/// @param variant A `VARIANT_BOOL`.
ACTION_TEMPLATE(SetPropVariantToBool, HAS_1_TEMPLATE_PARAMS(int, idx), AND_1_VALUE_PARAMS(variantBool)) {  // NOLINT(cppcoreguidelines-special-member-functions, misc-non-private-member-variables-in-classes)
	using idx_type = typename std::tuple_element<idx, args_type>::type;                                    // NOLINT(readability-identifier-naming)
	static_assert(std::is_base_of_v<PROPVARIANT, std::remove_pointer_t<std::remove_pointer_t<idx_type>>>);
	static_assert(std::is_same_v<VARIANT_BOOL, variantBool_type>);

	PROPVARIANT* const ppv = t::get<idx>(args);
	ppv->boolVal = variantBool;
	ppv->vt = VT_BOOL;
}

/// @brief Action for setting a pointer to a `PROPVARIANT` to a `BSTR` value.
/// @details Usage: `SetPropVariantToBool<1>("value")`. The `PROPVARIANT` MUST NOT be null.
/// @tparam idx 0-based index of the argument.
/// @param wsz A wide character string.
ACTION_TEMPLATE(SetPropVariantToBSTR, HAS_1_TEMPLATE_PARAMS(int, idx), AND_1_VALUE_PARAMS(wsz)) {  // NOLINT(cppcoreguidelines-special-member-functions, misc-non-private-member-variables-in-classes)
	using idx_type = typename std::tuple_element<idx, args_type>::type;                            // NOLINT(readability-identifier-naming)
	static_assert(std::is_base_of_v<PROPVARIANT, std::remove_pointer_t<std::remove_pointer_t<idx_type>>>);

	PROPVARIANT* const ppv = t::get<idx>(args);
	PROPVARIANT pv;
	HRESULT hr = InitPropVariantFromString(wsz, &pv);
	if (FAILED(hr)) [[unlikely]] {
		throw std::system_error(hr, std::system_category(), "InitPropVariantFromString");
	}
	hr = PropVariantChangeType(ppv, pv, 0, VARENUM::VT_BSTR);
	if (FAILED(hr)) [[unlikely]] {
		throw std::system_error(hr, std::system_category(), "PropVariantChangeType");
	}
}

/// @brief Action for setting a pointer to a `PROPVARIANT` to `VT_EMPTY`.
/// @details Usage: `SetPropVariantToEmpty<1>()`. The `PROPVARIANT` MUST NOT be null.
/// @tparam idx 0-based index of the argument.
ACTION_TEMPLATE(SetPropVariantToEmpty, HAS_1_TEMPLATE_PARAMS(int, idx), AND_0_VALUE_PARAMS()) {  // NOLINT(cppcoreguidelines-special-member-functions)
	using idx_type = typename std::tuple_element<idx, args_type>::type;                          // NOLINT(readability-identifier-naming)
	static_assert(std::is_base_of_v<PROPVARIANT, std::remove_pointer_t<idx_type>>);

	PROPVARIANT* const ppv = t::get<idx>(args);
	const HRESULT hr = PropVariantClear(ppv);
	if (FAILED(hr)) [[unlikely]] {
		throw std::system_error(hr, std::system_category(), "PropVariantClear");
	}
}

/// @brief Action for setting pointer to a `PROPVARIANT` to an `IStream` value.
/// @details Usage: `SetPropVariantToBool<1>(pStream)`.  The `PROPVARIANT` MUST NOT be null.
/// @tparam idx 0-based index of the argument.
/// @param variant An object of type `IStream`.
ACTION_TEMPLATE(SetPropVariantToStream, HAS_1_TEMPLATE_PARAMS(int, idx), AND_1_VALUE_PARAMS(pStream)) {  // NOLINT(cppcoreguidelines-special-member-functions, misc-non-private-member-variables-in-classes)
	using idx_type = typename std::tuple_element<idx, args_type>::type;                                  // NOLINT(readability-identifier-naming)
	static_assert(std::is_base_of_v<PROPVARIANT, std::remove_pointer_t<idx_type>>);

	PROPVARIANT* const ppv = t::get<idx>(args);
	pStream->AddRef();
	ppv->pStream = pStream;
	ppv->vt = VT_STREAM;
}

/// @brief Action for setting pointer to a `PROPVARIANT` to an `IStream` value.
/// @details Usage: `SetPropVariantToBool<1>(pStream)`.  The `PROPVARIANT` MUST NOT be null.
/// @tparam idx 0-based index of the argument.
/// @param variant An object of type `IStream`.
ACTION_TEMPLATE(SetPropVariantToUInt32, HAS_1_TEMPLATE_PARAMS(int, idx), AND_1_VALUE_PARAMS(value)) {  // NOLINT(cppcoreguidelines-special-member-functions, misc-non-private-member-variables-in-classes)
	using idx_type = typename std::tuple_element<idx, args_type>::type;                                // NOLINT(readability-identifier-naming)
	static_assert(std::is_base_of_v<PROPVARIANT, std::remove_pointer_t<idx_type>>);

	PROPVARIANT* const ppv = t::get<idx>(args);
	ppv->ulVal = value;
	ppv->vt = VARENUM::VT_UI4;
}

}  // namespace m4t

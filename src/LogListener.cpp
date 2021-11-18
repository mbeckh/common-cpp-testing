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

#include "m4t/LogListener.h"

#include "m4t/m4t.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <windows.h>
#include <detours_gmock.h>
#include <evntprov.h>

#include <algorithm>
#include <regex>
#include <stdexcept>
#include <type_traits>

namespace m4t {

namespace t = testing;

#undef WIN32_FUNCTIONS
#define WIN32_FUNCTIONS(fn_)                                                                                                                                                                         \
	fn_(1, void, WINAPI, OutputDebugStringA,                                                                                                                                                         \
	    (LPCSTR lpOutputString),                                                                                                                                                                     \
	    (lpOutputString),                                                                                                                                                                            \
	    nullptr);                                                                                                                                                                                    \
	fn_(8, ULONG, __stdcall, EventWriteEx,                                                                                                                                                           \
	    (REGHANDLE RegHandle, PCEVENT_DESCRIPTOR EventDescriptor, ULONG64 Filter, ULONG Flags, LPCGUID ActivityId, LPCGUID RelatedActivityId, ULONG UserDataCount, PEVENT_DATA_DESCRIPTOR UserData), \
	    (RegHandle, EventDescriptor, Filter, Flags, ActivityId, RelatedActivityId, UserDataCount, UserData),                                                                                         \
	    nullptr)

class LogListener::Impl {
public:
	auto& GetMock() noexcept {
		return m_win32;
	}

public:
	DTGM_API_MOCK(m_win32, WIN32_FUNCTIONS);
};

namespace {

const std::regex& GetLineRegex() {
	static const std::regex kLineRegex("^\\[(.+?)\\] \\[\\d+\\] (.+)\n\tat .+\\(\\d+\\) \\(\\w+\\)\n((?:\tcaused by: .+\n\t\tat .+\\(\\d+\\) \\(\\w+\\)\n)*)$", std::regex_constants::optimize);
	return kLineRegex;
}

const std::regex& GetCauseRegex() {
	static const std::regex kCauseRegex("^\tcaused by: (.+)\n\t\tat .+\\(\\d+\\) \\(\\w+\\)\n", std::regex_constants::optimize);
	return kCauseRegex;
}

void CallDebug(LogListener& listener, const std::string& level, std::sregex_token_iterator& it) {
	// call in reverse order (i.e. same order as Event, first match is whole string, second is level (-> ignore)
	if (it != std::sregex_token_iterator()) {
		const std::string str = it->str();
		CallDebug(listener, level, ++it);
		listener.Debug(level, str);
	}
}

LogListenerMode operator&(const LogListenerMode lhs, const LogListenerMode rhs) {
	return static_cast<LogListenerMode>(static_cast<std::underlying_type_t<LogListenerMode>>(lhs) & static_cast<std::underlying_type_t<LogListenerMode>>(rhs));
}

}  // namespace


LogListener::LogListener(const LogListenerMode mode)
    : m_impl(std::make_unique<Impl>()) {
	ON_CALL(m_impl->GetMock(), OutputDebugStringA(m4t::MatchesRegex(GetLineRegex())))
	    .WillByDefault(t::Invoke([this](const LPCSTR lpOutputString) {
		    std::cmatch match;
		    if (!std::regex_match(lpOutputString, match, GetLineRegex())) {
			    throw std::invalid_argument("regex mismatch");
		    }
		    const std::string level = match.str(1);
		    const std::string cause = match.str(3);
		    std::sregex_token_iterator it = std::sregex_token_iterator(cause.cbegin(), cause.cend(), GetCauseRegex(), 1);
		    CallDebug(*this, level, it);
		    Debug(level, match.str(2));
		    return m_impl->GetMock().DTGM_Real_OutputDebugStringA(lpOutputString);
	    }));

	ON_CALL(m_impl->GetMock(), EventWriteEx)
	    .WillByDefault(t::Invoke([this](const REGHANDLE regHandle, const EVENT_DESCRIPTOR* const eventDescriptor, const ULONG64 filter, const ULONG flags, const GUID* activityId, const GUID* const relatedActivityId, const ULONG userDataCount, EVENT_DATA_DESCRIPTOR* const userData) {
		    // ignore file name and line which are added by m3c::Log automatically
		    const std::uint32_t userArgCount = std::max<std::uint32_t>(userDataCount, 2) - 2;
		    Event(eventDescriptor->Id, eventDescriptor->Level, eventDescriptor->Keyword, userArgCount);
		    for (std::uint32_t i = 0; i < userArgCount; ++i) {
			    EventArg(i, userData[i].Size, reinterpret_cast<const void*>(userData[i].Ptr));  // NOLINT(performance-no-int-to-ptr): API provides pointer as integer value.
		    }
		    return m_impl->GetMock().DTGM_Real_EventWriteEx(regHandle, eventDescriptor, filter, flags, activityId, relatedActivityId, userDataCount, userData);
	    }));

	EXPECT_CALL(*this, Debug).Times((mode & LogListenerMode::kStrictDebug) == LogListenerMode::kStrictDebug ? t::Exactly(0) : t::AnyNumber());
	EXPECT_CALL(*this, Event).Times((mode & LogListenerMode::kStrictEvent) == LogListenerMode::kStrictEvent ? t::Exactly(0) : t::AnyNumber());
	EXPECT_CALL(*this, EventArg).Times(t::AnyNumber());  // calls are always allowed
}

// required to delete std::unique_ptr with incomplete type
LogListener::~LogListener() = default;

}  // namespace m4t

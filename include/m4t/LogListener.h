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
#pragma once

#include <gmock/gmock.h>

#include <windows.h>

#include <cstdint>
#include <memory>
#include <string>

namespace m4t {

enum class LogListenerMode : std::uint8_t {
	kLazy = 0,
	kStrictEvent = 1,
	kStrictDebug = 2,
	kStrictAll = kStrictEvent | kStrictDebug
};

class LogListener {
public:
	LogListener(LogListenerMode mode = LogListenerMode::kLazy);  // NOLINT(google-explicit-constructor): Allow configuration in declaration in classes.
	~LogListener();

public:
	MOCK_METHOD(void, Debug, (const std::string&, const std::string&), (const));
	MOCK_METHOD(void, Event, (USHORT eventId, UCHAR level, ULONGLONG keyword, ULONG argCount), (const));
	MOCK_METHOD(void, EventArg, (ULONG index, ULONG size, const void* ptr), (const));

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};

}  // namespace m4t

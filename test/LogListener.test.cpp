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

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>  // IWYU pragma: keep
#include <gtest/gtest.h>

#include <windows.h>
#include <evntprov.h>

#include <cstdint>
#include <string>

namespace m4t::test {
namespace {

namespace t = testing;

TEST(LogListener, NoLogging) {
	const LogListener log;

	EXPECT_CALL(log, Debug).Times(0);
	EXPECT_CALL(log, Event).Times(0);
	EXPECT_CALL(log, EventArg).Times(0);
}

TEST(LogListener, Debug_Causes0) {
	const LogListener log;

	EXPECT_CALL(log, Debug).Times(0);
	EXPECT_CALL(log, Debug("MyLevel", "MyMessage"));
	EXPECT_CALL(log, Event).Times(0);
	EXPECT_CALL(log, EventArg).Times(0);

	OutputDebugStringA("[MyLevel] [1234] MyMessage\n\tat file.cpp(99) (MyFunction)\n");
}

TEST(LogListener, Debug_Causes1) {
	const LogListener log;

	const t::InSequence s;
	EXPECT_CALL(log, Debug).Times(0);
	EXPECT_CALL(log, Debug("MyLevel", "MyCause"));
	EXPECT_CALL(log, Debug("MyLevel", "MyMessage"));
	EXPECT_CALL(log, Event).Times(0);
	EXPECT_CALL(log, EventArg).Times(0);

	OutputDebugStringA("[MyLevel] [1234] MyMessage\n\tat file.cpp(99) (MyFunction)\n\tcaused by: MyCause\n\t\tat file.cpp(98) (MyCauseFunction)\n");
}

TEST(LogListener, Debug_Causes2) {
	const LogListener log;

	const t::InSequence s;
	EXPECT_CALL(log, Debug).Times(0);
	EXPECT_CALL(log, Debug("MyLevel", "MyRootCause"));
	EXPECT_CALL(log, Debug("MyLevel", "MyCause"));
	EXPECT_CALL(log, Debug("MyLevel", "MyMessage"));
	EXPECT_CALL(log, Event).Times(0);
	EXPECT_CALL(log, EventArg).Times(0);

	OutputDebugStringA("[MyLevel] [1234] MyMessage\n\tat file.cpp(99) (MyFunction)\n\tcaused by: MyCause\n\t\tat file.cpp(98) (MyCauseFunction)\n\tcaused by: MyRootCause\n\t\tat file.cpp(97) (MyRootCauseFunction)\n");
}

TEST(LogListener, Event_Data0) {
	constexpr char kFile[] = "file.cpp";
	constexpr std::uint32_t kLine = 99;
	const LogListener log;

	EXPECT_CALL(log, Debug).Times(0);
	EXPECT_CALL(log, Event).Times(0);
	EXPECT_CALL(log, Event(1, 99, 1024, 0));
	EXPECT_CALL(log, EventArg).Times(0);

	EVENT_DESCRIPTOR event;
	EventDescCreate(&event, 1, 0, 0, 99, 0, 0, 1024);

	EVENT_DATA_DESCRIPTOR data[2];
	EventDataDescCreate(&data[0], kFile, sizeof(kFile));
	EventDataDescCreate(&data[1], &kLine, sizeof(kLine));

	EventWriteEx(0, &event, 0, 0, nullptr, nullptr, 2, data);
}

TEST(LogListener, Event_Data1) {
	constexpr char kFile[] = "file.cpp";
	constexpr std::uint32_t kLine = 99;
	LogListener log;

	EXPECT_CALL(log, Debug).Times(0);
	EXPECT_CALL(log, Event).Times(0);
	EXPECT_CALL(log, EventArg).Times(0);
	EXPECT_CALL(log, Event(1, 99, 1024, 1));
	EXPECT_CALL(log, EventArg(0, sizeof(void*), &log));

	EVENT_DESCRIPTOR event;
	EventDescCreate(&event, 1, 0, 0, 99, 0, 0, 1024);

	EVENT_DATA_DESCRIPTOR data[3];
	EventDataDescCreate(&data[0], &log, sizeof(void*));
	EventDataDescCreate(&data[1], kFile, sizeof(kFile));
	EventDataDescCreate(&data[2], &kLine, sizeof(kLine));

	EventWriteEx(0, &event, 0, 0, nullptr, nullptr, 3, data);
}

TEST(LogListener, Event_Data2) {
	constexpr char kFile[] = "file.cpp";
	constexpr std::uint32_t kLine = 99;
	constexpr char kCharData = 'a';

	LogListener log;

	EXPECT_CALL(log, Debug).Times(0);
	EXPECT_CALL(log, Event).Times(0);
	EXPECT_CALL(log, EventArg).Times(0);
	EXPECT_CALL(log, Event(1, 99, 1024, 2));
	EXPECT_CALL(log, EventArg(0, sizeof(void*), &log));
	EXPECT_CALL(log, EventArg(1, sizeof(char), &kCharData));

	EVENT_DESCRIPTOR event;
	EventDescCreate(&event, 1, 0, 0, 99, 0, 0, 1024);

	EVENT_DATA_DESCRIPTOR data[4];
	EventDataDescCreate(&data[0], &log, sizeof(void*));
	EventDataDescCreate(&data[1], &kCharData, sizeof(kCharData));
	EventDataDescCreate(&data[2], kFile, sizeof(kFile));
	EventDataDescCreate(&data[3], &kLine, sizeof(kLine));

	EventWriteEx(0, &event, 0, 0, nullptr, nullptr, 4, data);
}


//
// Strict / Non-Strict
//

TEST(LogListener, Debug_StrictLazyAndNotExpected_Ok) {
	LogListener log(LogListenerMode::kLazy);

	OutputDebugStringA("[MyLevel] [1234] MyMessage\n\tat file.cpp(99) (MyFunction)\n");

	EXPECT_TRUE(t::Mock::VerifyAndClearExpectations(&log));
}

TEST(LogListener, Debug_StrictEventAndNotExpected_Ok) {
	LogListener log(LogListenerMode::kStrictEvent);

	OutputDebugStringA("[MyLevel] [1234] MyMessage\n\tat file.cpp(99) (MyFunction)\n");

	EXPECT_TRUE(t::Mock::VerifyAndClearExpectations(&log));
}

TEST(LogListener, Debug_StrictDebugAndNotExpected_Error) {
	const LogListener log(LogListenerMode::kStrictDebug);

	EXPECT_NONFATAL_FAILURE(OutputDebugStringA("[MyLevel] [1234] MyMessage\n\tat file.cpp(99) (MyFunction)\n"), "called more times");
}

TEST(LogListener, Debug_StrictAllAndNotExpected_Error) {
	const LogListener log(LogListenerMode::kStrictAll);

	EXPECT_NONFATAL_FAILURE(OutputDebugStringA("[MyLevel] [1234] MyMessage\n\tat file.cpp(99) (MyFunction)\n"), "called more times");
}

TEST(LogListener, Debug_StrictDebugAndExpected_Ok) {
	LogListener log(LogListenerMode::kStrictDebug);
	EXPECT_CALL(log, Debug(t::_, "MyMessage"));

	OutputDebugStringA("[MyLevel] [1234] MyMessage\n\tat file.cpp(99) (MyFunction)\n");

	EXPECT_TRUE(t::Mock::VerifyAndClearExpectations(&log));
}

TEST(LogListener, Event_StrictLazyAndNotExpected_Ok) {
	LogListener log(LogListenerMode::kLazy);

	constexpr char kFile[] = "file.cpp";
	constexpr std::uint32_t kLine = 99;

	EVENT_DESCRIPTOR event;
	EventDescCreate(&event, 1, 0, 0, 99, 0, 0, 1024);

	EVENT_DATA_DESCRIPTOR data[3];
	EventDataDescCreate(&data[0], &log, sizeof(void*));
	EventDataDescCreate(&data[1], kFile, sizeof(kFile));
	EventDataDescCreate(&data[2], &kLine, sizeof(kLine));

	EventWriteEx(0, &event, 0, 0, nullptr, nullptr, 3, data);

	EXPECT_TRUE(t::Mock::VerifyAndClearExpectations(&log));
}

TEST(LogListener, Event_StrictDebugAndNotExpected_Ok) {
	LogListener log(LogListenerMode::kStrictDebug);

	constexpr char kFile[] = "file.cpp";
	constexpr std::uint32_t kLine = 99;

	EVENT_DESCRIPTOR event;
	EventDescCreate(&event, 1, 0, 0, 99, 0, 0, 1024);

	EVENT_DATA_DESCRIPTOR data[3];
	EventDataDescCreate(&data[0], &log, sizeof(void*));
	EventDataDescCreate(&data[1], kFile, sizeof(kFile));
	EventDataDescCreate(&data[2], &kLine, sizeof(kLine));

	EventWriteEx(0, &event, 0, 0, nullptr, nullptr, 3, data);

	EXPECT_TRUE(t::Mock::VerifyAndClearExpectations(&log));
}

TEST(LogListener, Event_StrictEventAndNotExpected_Error) {
	LogListener log(LogListenerMode::kStrictEvent);

	constexpr char kFile[] = "file.cpp";
	constexpr std::uint32_t kLine = 99;

	EVENT_DESCRIPTOR event;
	EventDescCreate(&event, 1, 0, 0, 99, 0, 0, 1024);

	EVENT_DATA_DESCRIPTOR data[3];
	EventDataDescCreate(&data[0], &log, sizeof(void*));
	EventDataDescCreate(&data[1], kFile, sizeof(kFile));
	EventDataDescCreate(&data[2], &kLine, sizeof(kLine));

	EXPECT_NONFATAL_FAILURE(EventWriteEx(0, &event, 0, 0, nullptr, nullptr, 3, data), "called more times");
}

TEST(LogListener, Event_StrictAllAndNotExpected_Error) {
	LogListener log(LogListenerMode::kStrictAll);

	constexpr char kFile[] = "file.cpp";
	constexpr std::uint32_t kLine = 99;

	EVENT_DESCRIPTOR event;
	EventDescCreate(&event, 1, 0, 0, 99, 0, 0, 1024);

	EVENT_DATA_DESCRIPTOR data[3];
	EventDataDescCreate(&data[0], &log, sizeof(void*));
	EventDataDescCreate(&data[1], kFile, sizeof(kFile));
	EventDataDescCreate(&data[2], &kLine, sizeof(kLine));

	EXPECT_NONFATAL_FAILURE(EventWriteEx(0, &event, 0, 0, nullptr, nullptr, 3, data), "called more times");
}

TEST(LogListener, Event_StrictEventAndExpected_Ok) {
	LogListener log(LogListenerMode::kStrictEvent);

	EXPECT_CALL(log, Event(1, 99, 1024, 1));

	constexpr char kFile[] = "file.cpp";
	constexpr std::uint32_t kLine = 99;

	EVENT_DESCRIPTOR event;
	EventDescCreate(&event, 1, 0, 0, 99, 0, 0, 1024);

	EVENT_DATA_DESCRIPTOR data[3];
	EventDataDescCreate(&data[0], &log, sizeof(void*));
	EventDataDescCreate(&data[1], kFile, sizeof(kFile));
	EventDataDescCreate(&data[2], &kLine, sizeof(kLine));

	EventWriteEx(0, &event, 0, 0, nullptr, nullptr, 3, data);

	EXPECT_TRUE(t::Mock::VerifyAndClearExpectations(&log));
}

}  // namespace
}  // namespace m4t::test

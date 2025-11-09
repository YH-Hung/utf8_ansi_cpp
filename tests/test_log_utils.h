#pragma once

#include <gtest/gtest.h>
#include <string>

namespace testlog {
inline void record_success_reason(const std::string& msg) {
    // Use a well-known key that our custom listener reads
    ::testing::Test::RecordProperty("success_reason", msg.c_str());
}
} // namespace testlog

// Convenience macro for brevity in tests
#define TEST_SUCCESS_REASON(MSG) ::testing::Test::RecordProperty("success_reason", (MSG))

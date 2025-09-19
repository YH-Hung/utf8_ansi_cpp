#include <gtest/gtest.h>
#include "library.h"

#include <string>
#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>

using namespace utf8ansi;

// helper to render bytes as hex
static std::string bytes_to_hex(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() * 3);
    for (unsigned char c : s) {
        out.push_back(hex[c >> 4]);
        out.push_back(hex[c & 0xF]);
        out.push_back(' ');
    }
    if (!out.empty()) out.pop_back();
    return out;
}

// ensure info logs are shown
static struct SpdlogInit {
    SpdlogInit() { spdlog::set_level(spdlog::level::info); }
} g_spdlog_init;

TEST(EncodingTest, Utf8ToBig5AndBack_Ascii) {
    std::string s = "Hello, 123!";
    std::string big5 = utf8_to_big5(s);
    std::string round = big5_to_utf8(big5);
    spdlog::info("big5 bytes: {}", bytes_to_hex(big5));
    spdlog::info("round bytes: {}", bytes_to_hex(round));
    EXPECT_EQ(round, s);
}

TEST(EncodingTest, Utf8ToBig5AndBack_Chinese) {
    // Common Chinese characters that are present in Big5
    // UTF-8 bytes for "中文": E4 B8 AD E6 96 87
    std::string s = "\xE4\xB8\xAD\xE6\x96\x87"; // U+4E2D U+6587
    std::string big5 = utf8_to_big5(s);
    std::string round = big5_to_utf8(big5);
    spdlog::info("big5 bytes: {}", bytes_to_hex(big5));
    spdlog::info("round bytes: {}", bytes_to_hex(round));
    EXPECT_EQ(round, s);
}

TEST(EncodingTest, Utf8ToBig5AndBack_MoreChineseSamples) {
    // A set of commonly used Traditional Chinese phrases likely present in Big5
    std::vector<std::string> samples = {
        "你好",
        "世界",
        "中文測試",
        "學習程式設計",
        "資料結構",
        "電腦與網路",
        "高雄",
        "香港",
        "測試中文123",
        "愛與和平"
    };
    for (const auto& s : samples) {
        std::string big5 = utf8_to_big5(s);
        std::string round = big5_to_utf8(big5);
        spdlog::info("sample: {} | big5: {}", s, bytes_to_hex(big5));
        EXPECT_EQ(round, s) << "Round-trip mismatch for: " << s;
    }
}

TEST(EncodingTest, Utf8ToBig5AndBack_ChinesePunctuation) {
    // Fullwidth punctuation commonly used in Chinese text
    // Note: These should exist in Big5 mapping
    std::string s = "「你好，世界！」（測試：中文、標點。）";
    std::string big5 = utf8_to_big5(s);
    std::string round = big5_to_utf8(big5);
    spdlog::info("punct big5 bytes: {}", bytes_to_hex(big5));
    EXPECT_EQ(round, s);
}

TEST(EncodingTest, EmbeddedNull_WithLengthOverload) {
    std::string s("A\0B", 3);
    // Convert UTF-8 to UTF-8 to validate pass-through with explicit length
    std::string out = convert_encoding(s.data(), s.size(), "UTF-8", "UTF-8");
    EXPECT_EQ(out, s);
}

TEST(EncodingTest, NullPointerInputThrows) {
    EXPECT_THROW({
        auto out = convert_encoding(nullptr, "UTF-8", "UTF-8");
        (void)out;
    }, std::invalid_argument);
}

TEST(EncodingTest, InvalidEncodingNameThrows) {
    EXPECT_THROW({
        auto out = convert_encoding("abc", "INVALID-ENC", "UTF-8");
        (void)out;
    }, std::runtime_error);
}

TEST(EncodingTest, InvalidUtf8BytesCauseError) {
    // Overlong sequence for '/': 0xC0 0xAF is not valid UTF-8
    std::string invalid(std::string("\xC0\xAF", 2));
    EXPECT_THROW({
        auto out = convert_encoding(std::string_view(invalid), "UTF-8", "UTF-8");
        (void)out;
    }, std::runtime_error);
}

TEST(EncodingTest, Utf8ToBig5_FailsOnSimplifiedChinese) {
    // Simplified characters are generally not representable in Big5
    std::vector<std::string> simplified = {
        "简体中文",
        "汉字",
        "学习编程"
    };
    for (const auto& s : simplified) {
        EXPECT_THROW({
            auto big5 = utf8_to_big5(s);
            (void)big5;
        }, std::runtime_error) << "Expected failure for simplified: " << s;
    }
}

TEST(EncodingTest, Utf8ToBig5_FailsOnEmoji) {
    std::string s = "你好😀"; // Emoji not representable in Big5
    EXPECT_THROW({
        auto big5 = utf8_to_big5(s);
        (void)big5;
    }, std::runtime_error);
}

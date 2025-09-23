#ifndef UTF8_ANSI_CPP_LIBRARY_H
#define UTF8_ANSI_CPP_LIBRARY_H

#include <string>
#include <string_view>

namespace utf8ansi {

// Convert from one encoding to another using ICU.
// Throws std::runtime_error on failure.
std::string convert_encoding(std::string_view input,
                             std::string_view from_encoding,
                             std::string_view to_encoding);

// Convenience helpers
std::string to_utf8(std::string_view input, std::string_view from_encoding);
std::string from_utf8(std::string_view utf8, std::string_view to_encoding);

// Big5 convenience helpers
std::string big5_to_utf8(std::string_view big5_bytes);
std::string utf8_to_big5(std::string_view utf8);

// "Direct" converters that avoid allocating a full UTF-16 intermediate buffer
// by using ICU's streaming API with a small pivot (ucnv_convertEx).
std::string big5_to_utf8_dr(std::string_view big5_bytes);
std::string utf8_to_big5_dr(std::string_view utf8);

// C-style input overloads (null-terminated)
std::string convert_encoding(const char* input,
                             std::string_view from_encoding,
                             std::string_view to_encoding);
std::string to_utf8(const char* input, std::string_view from_encoding);
std::string from_utf8(const char* utf8, std::string_view to_encoding);

// C-style input overloads with explicit length (for non-null-terminated data)
std::string convert_encoding(const char* input, std::size_t length,
                             std::string_view from_encoding,
                             std::string_view to_encoding);
std::string to_utf8(const char* input, std::size_t length, std::string_view from_encoding);
std::string from_utf8(const char* utf8, std::size_t length, std::string_view to_encoding);

} // namespace utf8ansi

#endif // UTF8_ANSI_CPP_LIBRARY_H
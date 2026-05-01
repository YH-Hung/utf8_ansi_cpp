#ifndef UTF8_ANSI_CPP_LIBRARY_H
#define UTF8_ANSI_CPP_LIBRARY_H

#include <string>
#include <string_view>

namespace utf8ansi {

// Controls behavior when the requested encoding transform may be unnecessary or
// would fail on the source bytes:
// - No  : always transform; throw on decode errors (default, existing behavior).
// - Yes : (1) if source bytes are already valid in to_encoding, return them
//             unchanged without performing any transform;
//         (2) otherwise transform normally, but if the from_encoding decode
//             fails, return the original source bytes unchanged.
enum class BypassOnDecodeError { No, Yes };

// Convert from one encoding to another using ICU.
// Throws std::runtime_error on failure.
// If bypass_on_decode_error is Yes, the function first checks whether the input
// is already valid in to_encoding and returns it unchanged if so; otherwise it
// transforms, falling back to the original input on from_encoding decode failure.
[[nodiscard]] std::string convert_encoding(std::string_view input,
                             std::string_view from_encoding,
                             std::string_view to_encoding,
                             BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);

// Convenience helpers
[[nodiscard]] std::string to_utf8(std::string_view input, std::string_view from_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string from_utf8(std::string_view utf8, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);

// Big5 convenience helpers
[[nodiscard]] std::string big5_to_utf8(std::string_view big5_bytes, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string utf8_to_big5(std::string_view utf8, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);

// "Direct" converters that avoid allocating a full UTF-16 intermediate buffer
// by using ICU's streaming API with a small pivot (ucnv_convertEx).
[[nodiscard]] std::string big5_to_utf8_dr(std::string_view big5_bytes, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string utf8_to_big5_dr(std::string_view utf8, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);

// C-style input overloads (null-terminated)
[[nodiscard]] std::string convert_encoding(const char* input,
                             std::string_view from_encoding,
                             std::string_view to_encoding,
                             BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string to_utf8(const char* input, std::string_view from_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string from_utf8(const char* utf8, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);

// Big5 helpers (C-style, null-terminated)
[[nodiscard]] std::string big5_to_utf8(const char* big5_bytes, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string utf8_to_big5(const char* utf8, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string big5_to_utf8_dr(const char* big5_bytes, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string utf8_to_big5_dr(const char* utf8, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);

// C-style input overloads with explicit length (for non-null-terminated data)
[[nodiscard]] std::string convert_encoding(const char* input, std::size_t length,
                             std::string_view from_encoding,
                             std::string_view to_encoding,
                             BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string to_utf8(const char* input, std::size_t length, std::string_view from_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string from_utf8(const char* utf8, std::size_t length, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);

// Big5 helpers (C-style with explicit length)
[[nodiscard]] std::string big5_to_utf8(const char* big5_bytes, std::size_t length, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string utf8_to_big5(const char* utf8, std::size_t length, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string big5_to_utf8_dr(const char* big5_bytes, std::size_t length, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);
[[nodiscard]] std::string utf8_to_big5_dr(const char* utf8, std::size_t length, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);

} // namespace utf8ansi

#endif // UTF8_ANSI_CPP_LIBRARY_H
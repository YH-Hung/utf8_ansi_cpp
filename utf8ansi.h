#ifndef UTF8_ANSI_CPP_LIBRARY_H
#define UTF8_ANSI_CPP_LIBRARY_H

#include <string>
#include <string_view>

namespace utf8ansi {

// Policy for handling characters/bytes that cannot be converted between
// encodings. The policy is applied symmetrically to both ICU callbacks
// (decode side `toU` and encode side `fromU`) where applicable.
//
// - Throw      : ICU STOP callbacks both ways; throw std::runtime_error on
//                any conversion error (default; previous `BypassOnDecodeError::No`).
// - Bypass     : ICU STOP callbacks both ways, plus a higher-level fallback
//                applied only on the decode side:
//                  (1) if source bytes are already valid in to_encoding, return
//                      them unchanged without performing any transform;
//                  (2) otherwise transform normally, but if the from_encoding
//                      decode fails, return the original source bytes unchanged.
//                Encode-side errors still throw under Bypass (matches the
//                previous `BypassOnDecodeError::Yes` contract).
// - Substitute : ICU UCNV_*_CALLBACK_SUBSTITUTE — invalid units are replaced
//                with the converter's substitution character/sequence
//                (e.g. U+FFFD on decode; '?' or 0x3F on encode for SBCS/DBCS).
// - Skip       : ICU UCNV_*_CALLBACK_SKIP — invalid units are dropped from
//                the output and conversion continues.
// - Escape     : ICU UCNV_*_CALLBACK_ESCAPE with the default style — invalid
//                units are emitted as a textual escape (e.g. `%UXXXX` / `%XNN`).
enum class InvalidCharPolicy { Throw, Bypass, Substitute, Skip, Escape };

// Convert from one encoding to another using ICU.
// Throws std::runtime_error on failure unless `policy` selects a non-throwing
// mode. See InvalidCharPolicy above for per-mode semantics.
[[nodiscard]] std::string convert_encoding(std::string_view input,
                             std::string_view from_encoding,
                             std::string_view to_encoding,
                             InvalidCharPolicy policy = InvalidCharPolicy::Throw);

// Convenience helpers
[[nodiscard]] std::string to_utf8(std::string_view input, std::string_view from_encoding, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string from_utf8(std::string_view utf8, std::string_view to_encoding, InvalidCharPolicy policy = InvalidCharPolicy::Throw);

// Big5 convenience helpers
[[nodiscard]] std::string big5_to_utf8(std::string_view big5_bytes, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string utf8_to_big5(std::string_view utf8, InvalidCharPolicy policy = InvalidCharPolicy::Throw);

// "Direct" converters that avoid allocating a full UTF-16 intermediate buffer
// by using ICU's streaming API with a small pivot (ucnv_convertEx).
[[nodiscard]] std::string big5_to_utf8_dr(std::string_view big5_bytes, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string utf8_to_big5_dr(std::string_view utf8, InvalidCharPolicy policy = InvalidCharPolicy::Throw);

// C-style input overloads (null-terminated)
[[nodiscard]] std::string convert_encoding(const char* input,
                             std::string_view from_encoding,
                             std::string_view to_encoding,
                             InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string to_utf8(const char* input, std::string_view from_encoding, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string from_utf8(const char* utf8, std::string_view to_encoding, InvalidCharPolicy policy = InvalidCharPolicy::Throw);

// Big5 helpers (C-style, null-terminated)
[[nodiscard]] std::string big5_to_utf8(const char* big5_bytes, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string utf8_to_big5(const char* utf8, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string big5_to_utf8_dr(const char* big5_bytes, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string utf8_to_big5_dr(const char* utf8, InvalidCharPolicy policy = InvalidCharPolicy::Throw);

// C-style input overloads with explicit length (for non-null-terminated data)
[[nodiscard]] std::string convert_encoding(const char* input, std::size_t length,
                             std::string_view from_encoding,
                             std::string_view to_encoding,
                             InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string to_utf8(const char* input, std::size_t length, std::string_view from_encoding, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string from_utf8(const char* utf8, std::size_t length, std::string_view to_encoding, InvalidCharPolicy policy = InvalidCharPolicy::Throw);

// Big5 helpers (C-style with explicit length)
[[nodiscard]] std::string big5_to_utf8(const char* big5_bytes, std::size_t length, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string utf8_to_big5(const char* utf8, std::size_t length, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string big5_to_utf8_dr(const char* big5_bytes, std::size_t length, InvalidCharPolicy policy = InvalidCharPolicy::Throw);
[[nodiscard]] std::string utf8_to_big5_dr(const char* utf8, std::size_t length, InvalidCharPolicy policy = InvalidCharPolicy::Throw);

} // namespace utf8ansi

#endif // UTF8_ANSI_CPP_LIBRARY_H

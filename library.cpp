#include "library.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <iterator>
#include <limits>

#include <unicode/ucnv.h>

namespace utf8ansi {

namespace {

/**
 * Safely convert std::size_t to int32_t for ICU APIs.
 * Throws std::runtime_error if the size exceeds INT32_MAX.
 */
int32_t safe_size_to_int32(const std::size_t size) {
    constexpr auto max_int32 = static_cast<std::size_t>(std::numeric_limits<int32_t>::max());
    if (size > max_int32) {
        throw std::runtime_error("Input size exceeds maximum supported size (2GB)");
    }
    return static_cast<int32_t>(size);
}

/**
 * Safely multiply size_t values, checking for overflow.
 * Throws std::runtime_error if the multiplication would overflow.
 */
std::size_t safe_multiply(const std::size_t a, const std::size_t b) {
    if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a) {
        throw std::runtime_error("Buffer size calculation overflow");
    }
    return a * b;
}

/**
 * Safely add size_t values, checking for overflow.
 * Throws std::runtime_error if the addition would overflow.
 */
std::size_t safe_add(const std::size_t a, const std::size_t b) {
    if (a > std::numeric_limits<std::size_t>::max() - b) {
        throw std::runtime_error("Buffer size calculation overflow");
    }
    return a + b;
}

struct UConverterHandle {
    // RAII holder for an ICU UConverter (character set converter).
    // - Acquires the converter in the constructor using an ICU encoding name or alias.
    // - Configures both "to Unicode" and "from Unicode" callbacks to STOP on errors,
    //   so invalid sequences cause an immediate failure instead of silent substitution.
    // - Releases the converter in the destructor.
    //
    // Usage: create as an automatic (stack) variable and pass get() to ICU APIs.
    // Thread-safety: do not share a single UConverter across threads.
    UConverter* conv{nullptr};
    
    // Delete copy and move operations to prevent accidental copying/moving
    UConverterHandle(const UConverterHandle&) = delete;
    UConverterHandle& operator=(const UConverterHandle&) = delete;
    UConverterHandle(UConverterHandle&&) = delete;
    UConverterHandle& operator=(UConverterHandle&&) = delete;
    explicit UConverterHandle(const std::string_view name) {
        UErrorCode status = U_ZERO_ERROR;
        conv = ucnv_open(std::string(name).c_str(), &status);
        if (U_FAILURE(status) || conv == nullptr) {
            throw std::runtime_error("Failed to open ICU converter: " + std::string(name));
        }
        // Configure ICU to stop on conversion errors (no substitution/leniency).
        UErrorCode s2 = U_ZERO_ERROR;
        ucnv_setToUCallBack(conv, UCNV_TO_U_CALLBACK_STOP, nullptr, nullptr, nullptr, &s2);
        if (U_FAILURE(s2)) {
            throw std::runtime_error("Failed to set ICU TO-UNICODE callback to STOP for: " + std::string(name));
        }
        s2 = U_ZERO_ERROR;
        ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_STOP, nullptr, nullptr, nullptr, &s2);
        if (U_FAILURE(s2)) {
            throw std::runtime_error("Failed to set ICU FROM-UNICODE callback to STOP for: " + std::string(name));
        }
    }
    ~UConverterHandle() {
        if (conv) ucnv_close(conv);
    }
    // Accessor for the underlying ICU handle; ownership remains with this wrapper.
    [[nodiscard]] UConverter* get() const { return conv; }
};

/**
 * Core conversion implementation using ICU in two pass preflight+convert steps:
 * 1) Source bytes -> UTF-16 (UChar) via ucnv_toUChars (preflight to size, then actual convert).
 * 2) UTF-16 -> target bytes via ucnv_fromUChars (preflight to size, then actual convert).
 *
 * Parameters:
 * - input: pointer to the source byte sequence; must not be null.
 * - length: number of bytes in input. Use -1 to indicate NUL-terminated input (ICU convention).
 * - from_encoding: ICU canonical or alias name of the source encoding.
 * - to_encoding: ICU canonical or alias name of the destination encoding.
 *
 * Throws std::invalid_argument if input is null.
 * Throws std::runtime_error on ICU errors during either phase.
 * Returns the converted bytes without a terminating NUL.
 */
std::string convert_encoding_impl(const char* input,
                                  const int32_t length,
                                  const std::string_view from_encoding,
                                  const std::string_view to_encoding) {
    if (input == nullptr) {
        if (length == 0) {
            return {};
        }
        throw std::invalid_argument("convert_encoding: input is null");
    }

    const UConverterHandle from(from_encoding);
    const UConverterHandle to(to_encoding);

    // Step 1: Convert from source bytes to UTF-16 (UChar)
    UErrorCode status = U_ZERO_ERROR;
    const int32_t uLen = ucnv_toUChars(from.get(), nullptr, 0, input, length, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) {
        throw std::runtime_error("ICU preflight toUChars failed for encoding: " + std::string(from_encoding));
    }
    status = U_ZERO_ERROR;
    std::vector<UChar> ubuf(static_cast<size_t>(uLen) + 1u);
    const int32_t uWritten = ucnv_toUChars(from.get(), ubuf.data(), uLen + 1, input, length, &status);
    if (U_FAILURE(status)) {
        throw std::runtime_error("ICU toUChars failed for encoding: " + std::string(from_encoding));
    }

    // Step 2: Convert from UTF-16 (UChar) to target bytes
    status = U_ZERO_ERROR;
    const int32_t outLen = ucnv_fromUChars(to.get(), nullptr, 0, ubuf.data(), uWritten, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) {
        throw std::runtime_error("ICU preflight fromUChars failed for encoding: " + std::string(to_encoding));
    }
    status = U_ZERO_ERROR;
    std::string out;
    out.resize(static_cast<size_t>(outLen));
    const int32_t written = ucnv_fromUChars(to.get(), out.data(), outLen, ubuf.data(), uWritten, &status);
    if (U_FAILURE(status)) {
        throw std::runtime_error("ICU fromUChars failed for encoding: " + std::string(to_encoding));
    }
    // No need to resize; ICU wrote exactly 'written' bytes, which should equal outLen
    // but to be safe, adjust when different
    if (written >= 0 && written < outLen) {
        out.resize(static_cast<size_t>(written));
    }

    return out;
}

/**
 * Streaming conversion using ICU ucnv_convertEx to avoid allocating a full UTF-16 buffer.
 *
 * This function converts incrementally through a small UTF-16 pivot buffer, growing the
 * output buffer as needed. Converters are configured to STOP on errors; any invalid input
 * results in an exception rather than silent substitution.
 *
 * Parameters:
 * - input: the source bytes to convert.
 * - from_encoding: ICU name (canonical or alias) of the source encoding.
 * - to_encoding: ICU name (canonical or alias) of the target encoding.
 * - initial_out_capacity: heuristic initial size for the output buffer; it will expand if required.
 *
 * Returns the converted bytes.
 * Throws std::invalid_argument if input.data() is null while input.size() != 0.
 * Throws std::runtime_error on ICU conversion errors.
 */
std::string convert_encoding_streaming(const std::string_view input,
                                              const std::string_view from_encoding,
                                              const std::string_view to_encoding,
                                              const std::size_t initial_out_capacity) {
    if (input.data() == nullptr && !input.empty()) {
        throw std::invalid_argument("convert_encoding_streaming: input is null but size != 0");
    }

    const UConverterHandle from(from_encoding);
    const UConverterHandle to(to_encoding);

    // Prepare output buffer with a heuristic initial capacity.
    std::string out;
    std::size_t cap = initial_out_capacity;
    if (cap < 16) cap = 16;
    out.resize(cap);
    char* target = out.data();
    const char* targetLimit = out.data() + out.size();

    const char* source = input.data();
    const char* const sourceLimit = input.data() + input.size();

    // Small pivot buffer for UTF-16 code units used internally by ICU.
    UChar pivot[256];
    UChar* pivotSource = pivot;
    UChar* pivotTarget = pivot;

    bool reset = true;

    for (;;) {
        UErrorCode status = U_ZERO_ERROR;
        const UBool flush = (source == sourceLimit) ? 1 : 0;
        ucnv_convertEx(
            /*targetCnv*/ to.get(),
            /*sourceCnv*/ from.get(),
            /*target*/ &target,
            /*targetLimit*/ targetLimit,
            /*source*/ &source,
            /*sourceLimit*/ sourceLimit,
            /*pivotStart*/ pivot,
            /*pivotSource*/ &pivotSource,
            /*pivotTarget*/ &pivotTarget,
            /*pivotLimit*/ pivot + std::size(pivot),
            /*reset*/ reset ? 1 : 0,
            /*flush*/ flush,
            /*status*/ &status);
        reset = false;

        if (status == U_BUFFER_OVERFLOW_ERROR) {
            // Grow the output buffer and continue.
            const auto used = static_cast<std::size_t>(target - out.data());
            const std::size_t newCap = safe_add(safe_multiply(out.size(), 2u), 16u);
            out.resize(newCap);
            target = out.data() + used;
            targetLimit = out.data() + out.size();
            continue;
        }
        if (U_FAILURE(status)) {
            throw std::runtime_error(
                std::string("ICU ucnv_convertEx failed for ") + std::string(from_encoding) + " -> " + std::string(to_encoding));
        }
        if (flush) {
            break; // all input consumed and pivot drained
        }
        // else, loop continues to consume remaining input
    }

    out.resize(static_cast<std::size_t>(target - out.data()));
    return out;
}

} // namespace

std::string convert_encoding(const std::string_view input,
                             const std::string_view from_encoding,
                             const std::string_view to_encoding) {
    return convert_encoding_impl(input.data(), safe_size_to_int32(input.size()), from_encoding, to_encoding);
}

std::string to_utf8(const std::string_view input, const std::string_view from_encoding) {
    return convert_encoding_impl(input.data(), safe_size_to_int32(input.size()), from_encoding, "UTF-8");
}

std::string from_utf8(const std::string_view utf8, const std::string_view to_encoding) {
    return convert_encoding_impl(utf8.data(), safe_size_to_int32(utf8.size()), "UTF-8", to_encoding);
}

std::string big5_to_utf8(const std::string_view big5_bytes) {
    return to_utf8(big5_bytes, "Big5");
}

std::string utf8_to_big5(const std::string_view utf8) {
    return from_utf8(utf8, "Big5");
}

std::string big5_to_utf8_dr(const std::string_view big5_bytes) {
    // Big5 bytes (1–2 per char) can expand up to ~3 bytes/char in UTF-8
    const std::size_t guess = safe_add(safe_multiply(big5_bytes.size(), 3u), 16u);
    return convert_encoding_streaming(big5_bytes, "Big5", "UTF-8", guess);
}

std::string utf8_to_big5_dr(const std::string_view utf8) {
    // UTF-8 (1–4 bytes/char) maps to Big5 (1–2 bytes/char); allocate generously
    const std::size_t guess = safe_add(safe_multiply(utf8.size(), 2u), 16u);
    return convert_encoding_streaming(utf8, "UTF-8", "Big5", guess);
}

std::string convert_encoding(const char* input,
                             const std::string_view from_encoding,
                             const std::string_view to_encoding) {
    return convert_encoding_impl(input, -1, from_encoding, to_encoding);
}

std::string to_utf8(const char* input, const std::string_view from_encoding) {
    return convert_encoding_impl(input, -1, from_encoding, "UTF-8");
}

std::string from_utf8(const char* utf8, const std::string_view to_encoding) {
    return convert_encoding_impl(utf8, -1, "UTF-8", to_encoding);
}

std::string convert_encoding(const char* input, const std::size_t length,
                             const std::string_view from_encoding,
                             const std::string_view to_encoding) {
    return convert_encoding_impl(input, safe_size_to_int32(length), from_encoding, to_encoding);
}

std::string to_utf8(const char* input, const std::size_t length, const std::string_view from_encoding) {
    return convert_encoding_impl(input, safe_size_to_int32(length), from_encoding, "UTF-8");
}

std::string from_utf8(const char* utf8, const std::size_t length, const std::string_view to_encoding) {
    return convert_encoding_impl(utf8, safe_size_to_int32(length), "UTF-8", to_encoding);
}

} // namespace utf8ansi
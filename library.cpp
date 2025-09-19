#include "library.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <unicode/ucnv.h>

namespace utf8ansi {

namespace {

struct UConverterHandle {
    UConverter* conv{nullptr};
    explicit UConverterHandle(std::string_view name) {
        UErrorCode status = U_ZERO_ERROR;
        conv = ucnv_open(std::string(name).c_str(), &status);
        if (U_FAILURE(status) || conv == nullptr) {
            throw std::runtime_error("Failed to open ICU converter: " + std::string(name));
        }
        // Configure to STOP on errors instead of substituting.
        UErrorCode s2 = U_ZERO_ERROR;
        ucnv_setToUCallBack(conv, UCNV_TO_U_CALLBACK_STOP, nullptr, nullptr, nullptr, &s2);
        s2 = U_ZERO_ERROR;
        ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_STOP, nullptr, nullptr, nullptr, &s2);
    }
    ~UConverterHandle() {
        if (conv) ucnv_close(conv);
    }
    UConverter* get() const { return conv; }
};

std::string convert_encoding_impl(const char* input,
                                  int32_t length,
                                  std::string_view from_encoding,
                                  std::string_view to_encoding) {
    if (input == nullptr) {
        throw std::invalid_argument("convert_encoding: input is null");
    }

    UConverterHandle from(from_encoding);
    UConverterHandle to(to_encoding);

    // Step 1: Convert from source bytes to UTF-16 (UChar)
    UErrorCode status = U_ZERO_ERROR;
    int32_t uLen = ucnv_toUChars(from.get(), nullptr, 0, input, length, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) {
        throw std::runtime_error("ICU preflight toUChars failed for encoding: " + std::string(from_encoding));
    }
    status = U_ZERO_ERROR;
    std::vector<UChar> ubuf(static_cast<size_t>(uLen) + 1u);
    int32_t uWritten = ucnv_toUChars(from.get(), ubuf.data(), uLen + 1, input, length, &status);
    if (U_FAILURE(status)) {
        throw std::runtime_error("ICU toUChars failed for encoding: " + std::string(from_encoding));
    }

    // Step 2: Convert from UTF-16 (UChar) to target bytes
    status = U_ZERO_ERROR;
    int32_t outLen = ucnv_fromUChars(to.get(), nullptr, 0, ubuf.data(), uWritten, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(status)) {
        throw std::runtime_error("ICU preflight fromUChars failed for encoding: " + std::string(to_encoding));
    }
    status = U_ZERO_ERROR;
    std::string out;
    out.resize(static_cast<size_t>(outLen));
    int32_t written = ucnv_fromUChars(to.get(), out.data(), outLen, ubuf.data(), uWritten, &status);
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

// Streaming conversion using ICU ucnv_convertEx to avoid allocating a full UTF-16 buffer.
static std::string convert_encoding_streaming(std::string_view input,
                                              std::string_view from_encoding,
                                              std::string_view to_encoding,
                                              std::size_t initial_out_capacity) {
    if (input.data() == nullptr && input.size() != 0) {
        throw std::invalid_argument("convert_encoding_streaming: input is null but size != 0");
    }

    UConverterHandle from(from_encoding);
    UConverterHandle to(to_encoding);

    // Prepare output buffer with a heuristic initial capacity.
    std::string out;
    std::size_t cap = initial_out_capacity;
    if (cap < 16) cap = 16;
    out.resize(cap);
    char* target = out.data();
    const char* targetLimit = out.data() + out.size();

    const char* source = input.data();
    const char* sourceLimit = input.data() + input.size();

    // Small pivot buffer for UTF-16 code units used internally by ICU.
    UChar pivot[256];
    UChar* pivotSource = pivot;
    UChar* pivotTarget = pivot;

    bool reset = true;

    for (;;) {
        UErrorCode status = U_ZERO_ERROR;
        UBool flush = (source == sourceLimit);
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
            /*pivotLimit*/ pivot + (sizeof(pivot)/sizeof(pivot[0])),
            /*reset*/ (reset ? static_cast<UBool>(1) : static_cast<UBool>(0)),
            /*flush*/ static_cast<UBool>(flush),
            /*status*/ &status);
        reset = false;

        if (status == U_BUFFER_OVERFLOW_ERROR) {
            // Grow the output buffer and continue.
            std::size_t used = static_cast<std::size_t>(target - out.data());
            std::size_t newCap = out.size() * 2u + 16u;
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

std::string convert_encoding(std::string_view input,
                             std::string_view from_encoding,
                             std::string_view to_encoding) {
    return convert_encoding_impl(input.data(), static_cast<int32_t>(input.size()), from_encoding, to_encoding);
}

std::string to_utf8(std::string_view input, std::string_view from_encoding) {
    return convert_encoding_impl(input.data(), static_cast<int32_t>(input.size()), from_encoding, "UTF-8");
}

std::string from_utf8(std::string_view utf8, std::string_view to_encoding) {
    return convert_encoding_impl(utf8.data(), static_cast<int32_t>(utf8.size()), "UTF-8", to_encoding);
}

std::string big5_to_utf8(std::string_view big5_bytes) {
    return to_utf8(big5_bytes, "Big5");
}

std::string utf8_to_big5(std::string_view utf8) {
    return from_utf8(utf8, "Big5");
}

std::string big5_to_utf8_dr(std::string_view big5_bytes) {
    // Big5 bytes (1–2 per char) can expand up to ~3 bytes/char in UTF-8
    std::size_t guess = big5_bytes.size() * 3u + 16u;
    return convert_encoding_streaming(big5_bytes, "Big5", "UTF-8", guess);
}

std::string utf8_to_big5_dr(std::string_view utf8) {
    // UTF-8 (1–4 bytes/char) maps to Big5 (1–2 bytes/char); allocate generously
    std::size_t guess = utf8.size() * 2u + 16u;
    return convert_encoding_streaming(utf8, "UTF-8", "Big5", guess);
}

std::string convert_encoding(const char* input,
                             std::string_view from_encoding,
                             std::string_view to_encoding) {
    return convert_encoding_impl(input, -1, from_encoding, to_encoding);
}

std::string to_utf8(const char* input, std::string_view from_encoding) {
    return convert_encoding_impl(input, -1, from_encoding, "UTF-8");
}

std::string from_utf8(const char* utf8, std::string_view to_encoding) {
    return convert_encoding_impl(utf8, -1, "UTF-8", to_encoding);
}

std::string convert_encoding(const char* input, std::size_t length,
                             std::string_view from_encoding,
                             std::string_view to_encoding) {
    return convert_encoding_impl(input, static_cast<int32_t>(length), from_encoding, to_encoding);
}

std::string to_utf8(const char* input, std::size_t length, std::string_view from_encoding) {
    return convert_encoding_impl(input, static_cast<int32_t>(length), from_encoding, "UTF-8");
}

std::string from_utf8(const char* utf8, std::size_t length, std::string_view to_encoding) {
    return convert_encoding_impl(utf8, static_cast<int32_t>(length), "UTF-8", to_encoding);
}

} // namespace utf8ansi
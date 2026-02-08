# utf8-ansi-cpp

A tiny C++20 library that converts strings between UTF-8 and legacy ANSI encodings (such as Big5) using the ICU library.

## Purpose
This project provides simple, free (non-member) functions to convert:
- Any encoding → UTF-8
- UTF-8 → Any encoding
- Convenience helpers for Big5

Inputs can be std::string or C-style strings (const char*) with optional explicit length overloads for non-null-terminated data.

## Pre-requisites
- A C++20 compiler and CMake
- ICU (International Components for Unicode)

Installing ICU:
- macOS (Homebrew):
  - Install: `brew install icu4c`
  - icu4c is keg-only, so CMake may not find it automatically. You can hint CMake with one of:
    - `-DICU_ROOT=$(brew --prefix icu4c)`
    - or `-DCMAKE_PREFIX_PATH=$(brew --prefix icu4c)`
  - Alternatively, export flags so compilers/linkers can see headers and libs:
    - `export CPPFLAGS="-I$(brew --prefix icu4c)/include $CPPFLAGS"`
    - `export LDFLAGS="-L$(brew --prefix icu4c)/lib $LDFLAGS"`
- Ubuntu/Debian:
  - Install: `sudo apt-get update && sudo apt-get install -y libicu-dev`
  - CMake usually finds ICU automatically under `/usr`.
  - If it does not, explicitly hint the install prefix:
    - `-DICU_ROOT=/usr`
    - or `-DCMAKE_PREFIX_PATH=/usr`
  - Typical locations (for reference):
    - headers: `/usr/include`
    - libraries: `/usr/lib/x86_64-linux-gnu`
- Windows (vcpkg):
  - Install: `vcpkg install icu`
  - Configure CMake with the vcpkg toolchain: `-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg.cmake`

CMake uses `find_package(ICU)` internally. Use the hints above if ICU is not found.

## How to build
This repository is set up as a CMake project exposing a library target named `utf8_ansi_cpp`.

Example CMake invocation (generic):

```
cmake --fresh -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target utf8_ansi_cpp
```

If ICU is installed in a non-standard prefix, add:

```
cmake --fresh -S . -B build -DCMAKE_PREFIX_PATH=/path/to/icu/prefix
```

## Install

Install the library and header using CMake’s install step.

Option A: set the install prefix at configure time:
```
export MY_INSTALL_DIR=$HOME/.local

cmake --fresh -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR
cmake --build build --target utf8_ansi_cpp
cmake --install build
```

Option B: specify the prefix at install time:
```
cmake --fresh --build build --target utf8_ansi_cpp
cmake --install build --prefix /usr/local
```

What gets installed (GNUInstallDirs):
- Header: <prefix>/include/utf8ansi.h
- Shared library: <prefix>/lib (Linux/macOS: libutf8_ansi_cpp.*; Windows: utf8_ansi_cpp.dll + import lib)

Notes:
- If ICU is not in a standard location, use the same hints shown above when configuring.
- At runtime, ensure ICU’s shared libraries are discoverable:
  - Linux: add ICU’s lib dir to /etc/ld.so.conf.d and run ldconfig, or set LD_LIBRARY_PATH.
  - macOS: set DYLD_LIBRARY_PATH or install ICU to a standard prefix.
  - Windows: add ICU’s bin dir to PATH.

Staged installs (packaging):
```
DESTDIR=/tmp/stage cmake --install build --prefix /usr/local
```

## How to use
Include the header and link against the `utf8_ansi_cpp` library (and ICU).

```cpp
#include "utf8ansi.h"
#include <iostream>

int main() {
    using namespace utf8ansi;

    // Big5 → UTF-8
    std::string big5_bytes = /* ... */;
    std::string utf8 = big5_to_utf8(big5_bytes);

    // UTF-8 → Big5
    std::string utf8_text = u8"中文測試";
    std::string big5_out = utf8_to_big5(utf8_text);

    // Streaming-based (no full UTF-16 buffer materialization) variants using ICU ucnv_convertEx
    std::string utf8_dr = big5_to_utf8_dr(big5_bytes);
    std::string big5_out_dr = utf8_to_big5_dr(utf8_text);

    // Any encoding ↔ UTF-8
    std::string iso_8859_1 = /* ... */;
    std::string utf8_from_iso = to_utf8(iso_8859_1, "ISO-8859-1");
    std::string shift_jis = from_utf8(utf8_from_iso, "Shift_JIS");

    // Big5 helpers for C-strings (null-terminated)
    const char* big5_cstr = /* ... */;
    std::string utf8_from_big5_c = big5_to_utf8(big5_cstr);
    std::string big5_from_utf8_c = utf8_to_big5(utf8_text.c_str());

    // Big5 helpers with explicit length (non-null-terminated)
    const char* raw = /* ptr to bytes */;
    size_t len = /* size */;
    std::string utf8_from_big5_len = big5_to_utf8(raw, len);
    std::string big5_from_utf8_len = utf8_to_big5(utf8_text.data(), utf8_text.size());

    // Streaming-based (DR) C-style overloads
    std::string utf8_from_big5_dr_c = big5_to_utf8_dr(big5_cstr);
    std::string big5_from_utf8_dr_c = utf8_to_big5_dr(utf8_text.c_str());
    std::string utf8_from_big5_dr_len = big5_to_utf8_dr(raw, len);
    std::string big5_from_utf8_dr_len = utf8_to_big5_dr(utf8_text.data(), utf8_text.size());

    std::cout << utf8 << "\n";
}
```

### Bypassing decode errors (optional)
By default, all converters stop on invalid input and throw an exception. If you know that some inputs may be mislabeled (e.g., bytes are actually UTF-8 but the code path passes `"Big5"`), you can opt into a safe bypass that returns the original input unchanged when decoding fails:

```cpp
using namespace utf8ansi;

std::string bytes = /* maybe mislabeled as Big5 */;
// Try Big5 -> UTF-8, but if it isn't actually Big5, just return bytes unchanged.
auto out = big5_to_utf8(bytes, BypassOnDecodeError::Yes);

// Generic
auto out2 = convert_encoding(bytes, "Big5", "UTF-8", BypassOnDecodeError::Yes);
```

Notes:
- The bypass only applies to the decode step (source bytes -> UTF-16). If decoding succeeds, conversion proceeds normally.
- Encode-side errors (UTF-16 -> destination bytes) still throw.

## API documentation
All functions are free functions in the `utf8ansi` namespace.

By default, functions throw on decode errors. You can opt into bypassing decode errors by passing the optional `BypassOnDecodeError` parameter (defaults to `BypassOnDecodeError::No`). When set to `Yes`, if the source bytes cannot be decoded to UTF‑16 using `from_encoding`, the function returns the original input bytes unchanged.

- `std::string convert_encoding(std::string_view input, std::string_view from_encoding, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
  - Convert bytes from `from_encoding` to `to_encoding`.
- `std::string to_utf8(std::string_view input, std::string_view from_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
  - Convert bytes from `from_encoding` to UTF-8.
- `std::string from_utf8(std::string_view utf8, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
  - Convert UTF-8 bytes to `to_encoding`.
- Big5 helpers:
  - `std::string big5_to_utf8(std::string_view big5_bytes, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
  - `std::string utf8_to_big5(std::string_view utf8, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
  - `std::string big5_to_utf8_dr(std::string_view big5_bytes, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);` (streaming)
  - `std::string utf8_to_big5_dr(std::string_view utf8, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);` (streaming)
- C-style overloads (null-terminated `const char*`):
  - Generic:
    - `std::string convert_encoding(const char* input, std::string_view from_encoding, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
    - `std::string to_utf8(const char* input, std::string_view from_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
    - `std::string from_utf8(const char* utf8, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
  - Big5 helpers:
    - `std::string big5_to_utf8(const char* big5_bytes, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
    - `std::string utf8_to_big5(const char* utf8, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
    - `std::string big5_to_utf8_dr(const char* big5_bytes, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);` (streaming)
    - `std::string utf8_to_big5_dr(const char* utf8, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);` (streaming)
- C-style overloads with explicit length (non-null-terminated):
  - Generic:
    - `std::string convert_encoding(const char* input, std::size_t length, std::string_view from_encoding, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
    - `std::string to_utf8(const char* input, std::size_t length, std::string_view from_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
    - `std::string from_utf8(const char* utf8, std::size_t length, std::string_view to_encoding, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
  - Big5 helpers:
    - `std::string big5_to_utf8(const char* big5_bytes, std::size_t length, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
    - `std::string utf8_to_big5(const char* utf8, std::size_t length, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);`
    - `std::string big5_to_utf8_dr(const char* big5_bytes, std::size_t length, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);` (streaming)
    - `std::string utf8_to_big5_dr(const char* utf8, std::size_t length, BypassOnDecodeError bypass_on_decode_error = BypassOnDecodeError::No);` (streaming)

### Error handling
- All functions throw `std::runtime_error` on conversion errors. ICU converters are configured to STOP on errors (no silent substitution).
- You may opt to bypass decode errors by passing `BypassOnDecodeError::Yes`; in that case, if decoding the source bytes fails because they don't match `from_encoding`, the function returns the original input unchanged.
- For null-terminated C-string overloads (`const char*`), passing `nullptr` throws `std::invalid_argument`.
- For explicit-length overloads (`const char* ptr, std::size_t len`):
  - If `ptr == nullptr` and `len == 0`, the functions return an empty string.
  - If `ptr == nullptr` and `len > 0`, the functions throw `std::invalid_argument`.
- The same semantics apply to the streaming (DR) variants.

### Encoding names
Use standard ICU encoding names (e.g., `"UTF-8"`, `"Big5"`, `"Shift_JIS"`, `"ISO-8859-1"`). Names are case-insensitive.

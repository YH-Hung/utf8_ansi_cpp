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
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target utf8_ansi_cpp
```

If ICU is installed in a non-standard prefix, add:

```
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/icu/prefix
```

## How to use
Include the header and link against the `utf8_ansi_cpp` library (and ICU).

```cpp
#include "library.h"
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

## API documentation
All functions are free functions in the `utf8ansi` namespace.

- `std::string convert_encoding(std::string_view input, std::string_view from_encoding, std::string_view to_encoding);`
  - Convert bytes from `from_encoding` to `to_encoding`.
- `std::string to_utf8(std::string_view input, std::string_view from_encoding);`
  - Convert bytes from `from_encoding` to UTF-8.
- `std::string from_utf8(std::string_view utf8, std::string_view to_encoding);`
  - Convert UTF-8 bytes to `to_encoding`.
- Big5 helpers:
  - `std::string big5_to_utf8(std::string_view big5_bytes);`
  - `std::string utf8_to_big5(std::string_view utf8);`
  - `std::string big5_to_utf8_dr(std::string_view big5_bytes);` (streaming)
  - `std::string utf8_to_big5_dr(std::string_view utf8);` (streaming)
- C-style overloads (null-terminated `const char*`):
  - Generic:
    - `std::string convert_encoding(const char* input, std::string_view from_encoding, std::string_view to_encoding);`
    - `std::string to_utf8(const char* input, std::string_view from_encoding);`
    - `std::string from_utf8(const char* utf8, std::string_view to_encoding);`
  - Big5 helpers:
    - `std::string big5_to_utf8(const char* big5_bytes);`
    - `std::string utf8_to_big5(const char* utf8);`
    - `std::string big5_to_utf8_dr(const char* big5_bytes);` (streaming)
    - `std::string utf8_to_big5_dr(const char* utf8);` (streaming)
- C-style overloads with explicit length (non-null-terminated):
  - Generic:
    - `std::string convert_encoding(const char* input, std::size_t length, std::string_view from_encoding, std::string_view to_encoding);`
    - `std::string to_utf8(const char* input, std::size_t length, std::string_view from_encoding);`
    - `std::string from_utf8(const char* utf8, std::size_t length, std::string_view to_encoding);`
  - Big5 helpers:
    - `std::string big5_to_utf8(const char* big5_bytes, std::size_t length);`
    - `std::string utf8_to_big5(const char* utf8, std::size_t length);`
    - `std::string big5_to_utf8_dr(const char* big5_bytes, std::size_t length);` (streaming)
    - `std::string utf8_to_big5_dr(const char* utf8, std::size_t length);` (streaming)

### Error handling
- All functions throw `std::runtime_error` on conversion errors. ICU converters are configured to STOP on errors (no silent substitution).
- For null-terminated C-string overloads (`const char*`), passing `nullptr` throws `std::invalid_argument`.
- For explicit-length overloads (`const char* ptr, std::size_t len`):
  - If `ptr == nullptr` and `len == 0`, the functions return an empty string.
  - If `ptr == nullptr` and `len > 0`, the functions throw `std::invalid_argument`.
- The same semantics apply to the streaming (DR) variants.

### Encoding names
Use standard ICU encoding names (e.g., `"UTF-8"`, `"Big5"`, `"Shift_JIS"`, `"ISO-8859-1"`). Names are case-insensitive.

# md5

[![CI](https://github.com/Kronuz/md5/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/md5/actions/workflows/ci.yml)

A small C++20 implementation of the MD5 message-digest function, used here as a
fast content fingerprint / checksum for content-addressing, not as a security
primitive. Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).

## What it is

One class, `MD5`. You hand it bytes (a `std::string_view`, or a pointer and a
size) and it gives you back the 16-byte digest as a 32-character lowercase hex
string. You can hash a whole buffer in a single call, or stream it in chunk by
chunk with `add()` and finalize with `getHash()`. It is size-based, so it hashes
exactly the bytes you give it, including any embedded `\0`.

This is a digest / checksum primitive. MD5 is fine for content-addressing,
deduplication, and integrity checks against accidental change. It is not
collision-resistant against an adversary, so don't use it where you need a
security guarantee.

## Install

This is not header-only. It ships a compiled translation unit, `md5.cc`, plus
the header `md5.h`. You build and link the `.cc`; the header alone only declares
the `MD5` class. It requires C++20.

It has one dependency: [endian](https://github.com/Kronuz/endian), the sibling
header that maps the platform byte-order macros (`__BYTE_ORDER` / `__BIG_ENDIAN`)
onto a uniform set of names. `md5.cc` uses them to decide whether the 32-bit
words need byte-swapping on a big-endian host. CMake pulls it in for you.

With CMake `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  md5
  GIT_REPOSITORY https://github.com/Kronuz/md5.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(md5)

target_link_libraries(your_target PRIVATE md5::md5)
```

`CMakeLists.txt` requests `cxx_std_20` `PUBLIC` and links `endian` `PUBLIC`, so
`"endian.hh"` resolves on the include path with no extra wiring on your side.
Then:

```cpp
#include "md5.h"
```

The header keeps its original filename, so a codebase that already
`#include "md5.h"` just needs this repo on its include path.

## Usage

```cpp
#include "md5.h"

MD5 md5;

// Hash a string. Returns 32 lowercase hex characters.
md5(std::string_view("abc"));          // 900150983cd24fb0d6963f7d28e17f72
md5(std::string_view(""));             // d41d8cd98f00b204e9800998ecf8427e

// Hash an arbitrary byte buffer by pointer and length.
md5("How are you", 11);                // arbitrary data, 11 bytes

// It is size-based, so an embedded NUL is hashed, not treated as a terminator.
const char buf[] = {'a', '\0', 'b'};
md5(buf, sizeof(buf));

// Or stream it in and finalize. getHash() is repeatable; it does not consume
// the accumulated state.
MD5 stream;
stream.add("message ", 8);
stream.add("digest", 6);
stream.getHash();                      // f96b697d7cb7938d525a2f31aaf161d0
```

## API reference

```cpp
class MD5 {
  std::string operator()(const void* data, size_t numBytes);  // hash a buffer
  std::string operator()(std::string_view text);              // hash a string

  void add(const void* data, size_t numBytes);                // feed a chunk
  std::string getHash();                                      // 32 hex chars
  void        getHash(unsigned char buffer[16]);              // 16 raw bytes
  void reset();                                               // restart
};
```

The two call operators `reset()` for you, accumulate the bytes, and return the
hex digest. The streaming path is `add()` (called any number of times) followed
by `getHash()`. `getHash()` is repeatable: it finalizes a copy of the state, so
calling it twice yields the same digest and you can keep adding afterward.
`getHash(unsigned char[16])` gives you the raw bytes instead of hex.

## Build & test

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

The first configure fetches `endian` over the network. The test runs the
RFC 1321 vector suite, checks the `(void*, size)` overload against the
`string_view` one, exercises the streaming `add()` path (byte at a time, across a
block boundary), confirms `getHash()` is repeatable, reuses one instance via
`reset()`, hashes a buffer with an embedded NUL, and hashes a 1000-byte input
both in one shot and split in half. It prints `all md5 tests passed` and exits 0.

## Examples

[`examples/demo.cc`](examples/demo.cc) is a runnable tour. A top-level CMake build
produces it next to the test:

```sh
cmake -B build && cmake --build build && ./build/md5_demo
```

It hashes a couple of strings, hashes a raw buffer with an embedded NUL by
pointer and size, and shows the streaming `add()` path matching the one-shot
digest.

## Provenance

Extracted from [Xapiand](https://github.com/Kronuz/Xapiand). `md5.h` / `md5.cc`
were copied verbatim; the only change was wiring its one local include from
`"endian.h"` to `"endian.hh"` so it resolves against the standalone
[endian](https://github.com/Kronuz/endian) library through CMake. See
[ARCHITECTURE.md](ARCHITECTURE.md) for the design and [AGENTS.md](AGENTS.md) for
the repo map and invariants.

## License

MIT, Copyright (c) 2015-2019 Dubalu LLC; portions Copyright (c) 2014,2015 Stephan
Brumme. See [LICENSE](LICENSE).

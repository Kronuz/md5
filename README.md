# md5

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/md5/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/md5/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

MD5 for C++20 in one small class: one-call or streaming digests as 32 hex characters or 16 raw bytes.
Input of any size is buffered into 64-byte blocks and compressed in scalar C++,
and the digests are checked against RFC 1321 and OpenSSL. Reach for it when a
format or protocol requires MD5, such as `Content-MD5` or a legacy checksum.

## ✨ Highlights

- **One call or streaming.** `md5(data)` hashes a whole buffer; `add()` takes
  chunks and `getHash()` finalizes.
- **Hex or raw.** 32 lowercase hex characters, or 16 bytes into your buffer.
- **Any input.** The length is explicit, so embedded NULs count, and buffers at
  any alignment are read safely.
- **Repeatable reads.** `getHash()` finalizes a copy, so you can keep adding.
- **Checked** against the RFC 1321 test suite and, where OpenSSL is installed,
  against OpenSSL's MD5 on every length to 1100, random lengths to 70,000, and
  streams past 512 MiB.

## 🚀 Quick start

```cpp
#include "md5.h"

#include <cstdio>
#include <string_view>

int main() {
	MD5 md5;
	std::puts(md5(std::string_view("abc")).c_str());   // 900150983cd24fb0d6963f7d28e17f72

	md5.reset();
	md5.add("ab", 2);                                  // stream in any chunks
	md5.add("c", 1);
	std::puts(md5.getHash().c_str());                  // 900150983cd24fb0d6963f7d28e17f72

	unsigned char raw[MD5::HashBytes];                 // or the 16 raw bytes
	md5.getHash(raw);
	std::printf("%02x%02x\n", raw[0], raw[1]);         // 9001
}
```

[`examples/demo.cc`](examples/demo.cc) hashes a buffer with an embedded NUL and
shows streaming matching the one-shot digest.

## 📦 Installation

Requires a C++20 compiler and CMake 3.14 or newer; it fetches
[endian](https://github.com/Kronuz/endian) and compiles one source file. CI builds
it on Linux (x86_64, arm64), macOS (arm64, x86_64), and FreeBSD, plus ASan,
UBSan, and TSan builds; the Linux jobs also check it against OpenSSL.

```cmake
include(FetchContent)
FetchContent_Declare(md5
  GIT_REPOSITORY https://github.com/Kronuz/md5.git
  GIT_TAG        acdeff8f5a1c155090f73681763e2044862965e2)  # or main for the tip
FetchContent_MakeAvailable(md5)

target_link_libraries(my_app PRIVATE md5::md5)
```

```cpp
#include "md5.h"
```

## 🎯 When to use it

Use it to interoperate with something that requires MD5, without linking a
cryptography library.

Not for:

- **Security of any kind.** MD5 collisions are cheap to produce. Use
  [sha256](https://github.com/Kronuz/sha256) for fingerprints, and a reviewed
  cryptography library for anything adversarial.
- **The fastest MD5.** OpenSSL's is 1.1x to 1.6x faster once its algorithm
  lookup is out of the loop (see below).

## ⚡ Performance

`md5_hex_bench` times `MD5(data)` with its hex string against OpenSSL's MD5
through EVP, which the benchmark links when CMake finds OpenSSL. OpenSSL 3 looks
the algorithm up on every `EVP_DigestInit_ex(ctx, EVP_md5())`, so each cell is
this MD5 with hex / OpenSSL with hex and the lookup done once (`EVP_MD_fetch`) /
OpenSSL raw with `EVP_md5()` on every call. ns per call, one thread, best of two runs of
seven windows, Apple clang 17 with `-O3`; OpenSSL 3.5.9 on the Apple M4 Pro and
3.6.5 on the Intel Core i9-9980HK:

| Bytes | Apple M4 Pro | Intel Core i9-9980HK |
| ---: | ---: | ---: |
| 16 | 114 / 80 / 181 | 191 / 121 / 359 |
| 64 | 171 / 142 / 246 | 275 / 196 / 434 |
| 1,024 | 1,360 / 1,244 / 1,360 | 1,730 / 1,337 / 1,622 |
| 65,536 | 82,700 / 75,300 / 76,300 | 100,100 / 77,800 / 78,200 |
| 1,048,576 | 1,320,000 / 1,216,000 / 1,216,000 | 1,603,000 / 1,246,000 / 1,245,000 |

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMD5_BUILD_BENCH=ON
cmake --build build
./build/md5_hex_bench
```

The lookup costs OpenSSL 105 ns (M4 Pro) and 237 ns (i9) per call, which puts
this MD5 ahead of `EVP_md5()` up to 64 bytes.

## 📚 Documentation

- [docs/API.md](docs/API.md): the `MD5` class and caveats.
- [ARCHITECTURE.md](ARCHITECTURE.md): buffering, the rounds, and finalization.
- [CHANGELOG.md](CHANGELOG.md): what changed.
- [AGENTS.md](AGENTS.md): repository map and invariants.
- [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#md5): the catalog entry.

## 🛠️ Building and testing

```sh
cmake -B build
cmake --build build
ctest --test-dir build
```

`./build/md5_demo` runs the demo. Add `-DMD5_BUILD_BENCH=ON` to build
`md5_hex_bench`. Where CMake finds OpenSSL, `ctest` also runs `md5_openssl`.

## 🤝 Contributing

Issues and pull requests are welcome. Read [AGENTS.md](AGENTS.md) and the
catalog's [rules of engagement](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#rules-of-engagement)
first; CI must pass on every platform.

## 📄 License

MIT, Copyright (c) 2026 Germán Méndez Bravo and Copyright (c) 2015-2020 Dubalu
LLC. See [LICENSE](LICENSE). Based on Stephan Brumme's MD5, Copyright (c) 2014,
2015 Stephan Brumme ([terms](http://create.stephan-brumme.com/disclaimer.html)).
Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).

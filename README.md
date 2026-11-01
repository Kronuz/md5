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

`md5_hex_bench` times one call that takes a buffer and returns its 32-character hex string,
on one thread, against [OpenSSL](https://www.openssl.org/)'s MD5 through its EVP interface
(3.5.9 on the M4 Pro, 3.6.5 on the i9, both from Homebrew), which the benchmark links when
CMake finds OpenSSL. OpenSSL returns 16 raw bytes, so the first comparator converts them to
the same hex string, and each row checks that the two strings are equal. OpenSSL 3 looks the
algorithm up on every `EVP_DigestInit_ex(ctx, EVP_md5())`, so there are two OpenSSL columns:
`EVP_MD_fetch` looks `"MD5"` up once and the call is timed with the hex conversion, and
`EVP_md5()` is the call most code writes, timed for the digest alone. Cells are ns per call,
the better of two runs of seven windows; "× faster" is OpenSSL's time with hex over this
library's, so a value below 1 is a loss.

Apple M4 Pro (macOS 26), Apple clang 17.0.0 with `-O3 -DNDEBUG` (CMake Release):

| Bytes | `MD5` with hex | OpenSSL `EVP_MD_fetch` with hex | `MD5` × faster | OpenSSL `EVP_md5()`, digest only |
| ---: | ---: | ---: | ---: | ---: |
| 16 | 114 | 80 | 0.70 | 181 |
| 64 | 171 | 142 | 0.83 | 246 |
| 1,024 | 1,360 | 1,244 | 0.91 | 1,360 |
| 65,536 | 82,700 | 75,300 | 0.91 | 76,300 |
| 1,048,576 | 1,320,000 | 1,216,000 | 0.92 | 1,216,000 |

Intel Core i9-9980HK (macOS 15, x86-64), Apple clang 17.0.0 with `-O3 -DNDEBUG`:

| Bytes | `MD5` with hex | OpenSSL `EVP_MD_fetch` with hex | `MD5` × faster | OpenSSL `EVP_md5()`, digest only |
| ---: | ---: | ---: | ---: | ---: |
| 16 | 191 | 121 | 0.63 | 359 |
| 64 | 275 | 196 | 0.71 | 434 |
| 1,024 | 1,730 | 1,337 | 0.77 | 1,622 |
| 65,536 | 100,100 | 77,800 | 0.78 | 78,200 |
| 1,048,576 | 1,603,000 | 1,246,000 | 0.78 | 1,245,000 |

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMD5_BUILD_BENCH=ON
cmake --build build
./build/md5_hex_bench
```

With the algorithm fetched, OpenSSL is ahead on every row for the same hex string: 1.09 to
1.43 times as fast on the M4 Pro and 1.29 to 1.58 times on the i9. The lookup in `EVP_md5()`
costs OpenSSL 105 ns (M4 Pro) and 237 ns (i9) per call, which puts this MD5 ahead of that
call up to 64 bytes.

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

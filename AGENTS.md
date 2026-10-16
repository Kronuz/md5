# AGENTS.md

Working notes for agents modifying this repository. For the design read
`ARCHITECTURE.md`; for usage read `README.md`. This file covers the repo layout,
how to build and test, the invariants you must not break, and the traps that are
easy to fall into.

## Repo map

```
md5.h                        Declares the MD5 class (call operators, add(), getHash(), reset()) and its private state. Header. Verbatim from Xapiand.
md5.cc                       The implementation: block transform, streaming buffer, padding, length encoding, hex formatting. Includes "endian.hh" for the byte-order macros. From Xapiand, one include changed.
test/test.cc                 Runnable smoke test: RFC 1321 vectors, (void*,size) overload, streaming, repeatable getHash(), reset/reuse, embedded NUL, long multi-block input.
examples/demo.cc             A runnable tour (not a test).
CMakeLists.txt               Compiled library `md5` (+ alias md5::md5); FetchContent endian; CTest test `md5`.
LICENSE                      MIT, Copyright (c) 2015-2019 Dubalu LLC; portions (c) 2014,2015 Stephan Brumme.
README.md                    What it is, install, quick start.
docs/API.md                  The API reference.
ARCHITECTURE.md              Internal design of the block transform, padding, endianness, finalize-without-consume.
```

This is not header-only. `md5.cc` must be compiled and linked; the header only
declares the class. The CMake target links `endian` so `"endian.hh"` resolves.

## Build and run the test

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

The first configure fetches `endian` over the network (FetchContent,
`GIT_TAG main`). Expected output ends with `all md5 tests passed`, exit 0. The
test target is `md5_test`; the registered CTest name is `md5`. The test and demo
are only added when this repo is the top-level project (CMakeLists.txt), so
consumers vendoring it via `FetchContent` won't build them.

## Dependency

`md5.cc` includes `"endian.hh"` for the platform byte-order macros (`__BYTE_ORDER`
/ `__BIG_ENDIAN`), used to decide whether the 32-bit words need byte-swapping on a
big-endian host. That header lives in the sibling
[endian](https://github.com/Kronuz/endian) library, pulled in by CMake and linked
`PUBLIC` so the include path resolves. We track its tip with `GIT_TAG main`, like
the rest of the family. This is the library's only dependency; there was no
`log.h` / opts / config / `THROW` coupling in the original.

## Not cryptography

MD5 here is a content fingerprint / checksum for content-addressing,
deduplication, and integrity against accidental change. It is not
collision-resistant against an adversary. Keep the docs describing it as a digest
/ hashing function, not a security primitive. Don't add claims of cryptographic
strength.

## Conventions

- **C++20.** The target requests `cxx_std_20` `PUBLIC` to stay uniform with the
  sibling libraries. Don't drop the target below it.
- **Filenames are stable.** The header and source keep their original Xapiand
  names (`md5.h`, `md5.cc`) so a consumer that already `#include`s the header
  just needs this repo on the include path. Don't rename them.
- Tabs for indentation, double quotes in code, no em dashes in prose.
- MIT-licensed; keep the copyright header (Dubalu LLC and Stephan Brumme) on the
  source files.

## Load-bearing invariants

- **The digest must match the RFC 1321 vectors.** `test/test.cc` pins the full
  canonical suite plus the 1000-byte input. Any change to the transform, padding,
  or length encoding that breaks those vectors is a bug. Run the test.
- **`add()` carries state across block boundaries.** It tops up the 64-byte
  internal buffer, processes whole blocks straight from the caller's pointer, and
  parks the remainder. Streaming a message in arbitrary chunks must give the same
  digest as hashing it in one shot. The byte-at-a-time test case guards this.
- **`getHash()` does not consume the state.** `getHash(unsigned char[16])` saves
  the four state words, finalizes a copy, extracts the digest, then restores them.
  So `getHash()` is repeatable and `add()` can continue after it. Keep the
  save/restore; don't "simplify" it into finalizing in place.
- **The transform is little-endian.** `processBlock()` byte-swaps the input words
  only on a big-endian host, gated on `__BYTE_ORDER == __BIG_ENDIAN` from
  `endian.hh`. The 64-bit length tail is written low-byte-first by hand, so it is
  little-endian on any host. Don't change either path.

## Traps

- **`md5.h` declares `operator()(std::string_view)` but does not itself include
  `<string_view>`.** This is verbatim from Xapiand, where a transitive include
  supplied it. `md5.cc` includes `<string>` (via `md5.h`) which on the toolchains
  we build with also drags in `std::string_view`; the build is clean. If you ever
  see a `std::string_view` not-declared error in a stricter toolchain, add
  `#include <string_view>` to `md5.h` rather than working around it elsewhere.
- **Padding can spill into a second block.** When the message length lands such
  that the `0x80` byte plus the 8-byte length don't fit in the current block,
  `processBuffer()` uses an `extra[BlockSize]` block and runs `processBlock()`
  twice. The block-boundary test cases exercise this; don't drop the second
  `processBlock(extra)`.
- **It is size-based, not NUL-terminated.** It hashes exactly `numBytes` /
  `text.size()` bytes, including any embedded `\0`. Don't "optimize" it to stop at
  a NUL.

## Standalone vs. Xapiand

This is a standalone extraction from
[Xapiand](https://github.com/Kronuz/Xapiand). `md5.h` and `md5.cc` were copied
verbatim; the only change on extraction was the one local include in `md5.cc`,
from `"endian.h"` to `"endian.hh"`, so it resolves against the standalone endian
library through CMake instead of Xapiand's source tree. There is no other source
decoupling delta. Keep it that way; any change here should be reconcilable with
upstream as a plain edit.

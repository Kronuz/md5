# Changelog

Notable changes to `md5`, newest first. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## 2026-11-01

### Added

- `test/openssl.cc` (CTest `md5_openssl`, built when CMake finds OpenSSL; exit code 77 reads
  as skipped where OpenSSL has no MD5) checks every digest against OpenSSL's `EVP_md5`: every
  length from 0 to 1100, random lengths to 70,000 at every misalignment, one-shot calls,
  streaming in random chunks with `getHash()` at random cut points, and two streams past 2 MiB
  and 512 MiB, where the 64-bit message length first needs its fourth and fifth byte. Before it,
  MD5 had no external oracle. A mutant that drops the third, fourth or fifth byte of the
  message length, and one that keeps the length in 32 bits, pass `test/test.cc` (its longest
  input is 4096 bytes) and fail `md5_openssl`. Mutants of a round constant and of the hex
  conversion fail both.
- `md5_hex_bench` adds OpenSSL EVP columns when OpenSSL is found: the raw digest with the
  algorithm fetched once, the same plus the hex conversion, and the raw digest through
  `EVP_md5()`. It checks that the hex strings agree before timing. The numbers are in the
  README: OpenSSL is 1.1x to 1.6x faster with the lookup done once, and slower than this
  MD5 up to 64 bytes when `EVP_md5()` is passed on every call.
- CI installs `libssl-dev` so the Linux jobs run `md5_openssl`.

### Changed

- The README's single timing (115 ns at 16 bytes) is replaced by tables for the Apple M4 Pro
  and the Intel i9-9980HK.
- `md5_hex_bench` prints the OpenSSL version it linked, so a table names it.

## 2026-10-30

### Changed

- `MD5::getHash()` sizes the hex string once and fills two characters per byte,
  instead of appending one character at a time with `+=`. Output is identical (a
  differential test against the byte-at-a-time conversion covers lengths 0 to
  300). `MD5` on a 16-byte input went from about 161 ns to 115 ns (Apple M4 Pro,
  -O3, `md5_hex_bench`). The compression function is unchanged.

### Added

- `bench/hex_bench.cc` (`md5_hex_bench`, `-DMD5_BUILD_BENCH=ON`).

## 2026-10-28 and earlier

### Fixed

- `processBlock()` loads each input word with `memcpy` instead of reading the
  caller's buffer through a `uint32_t` pointer, so unaligned buffers are safe.
  `test_unaligned_input` covers offsets 0 to 15 against Python's `hashlib`.

# Changelog

Notable changes to `md5`, newest first. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

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

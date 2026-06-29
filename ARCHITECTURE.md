# Architecture

The internals of `md5`: how the MD5 message-digest function turns a byte stream
into a 16-byte digest, how the streaming buffer carries data across 64-byte
blocks, and why the one external dependency is `endian`. For usage see
`README.md`; for the repo map and invariants see `AGENTS.md`.

## Shape

A header that declares the class and a single compiled translation unit:

```
  md5.h    declares the MD5 class: the call operators, add(), getHash(), reset()
  md5.cc   the implementation: the block transform, padding, and hex formatting
```

`md5.h` carries no logic; it just declares the public surface and the private
state (the running 4-word hash, the 64-byte buffer, the byte counter). All the
work lives in `md5.cc`.

## The digest, in three parts

MD5 processes the message in 64-byte (512-bit) blocks, keeping a running 128-bit
state (four 32-bit words `a b c d`, seeded with the RFC 1321 constants in
`reset()`). Three pieces fit together:

- **`processBlock()`** runs the core transform over one 64-byte block: four
  rounds of 16 operations each, every operation mixing one of the 16 input words
  with a per-step sine-derived constant and a left-rotate. The mix functions
  `f1`..`f4` and the `rotate()` helper are file-local in an anonymous namespace.
- **`add()`** is the streaming front door. It tops up the internal buffer to a
  full block, processes any complete blocks straight from the caller's data, and
  parks the trailing bytes back in the buffer. So you can feed the message in
  arbitrary chunks and the block boundaries are handled for you.
- **`processBuffer()`** finalizes: it appends the mandatory `1` bit (a `0x80`
  byte), pads with zeros until the length lands at 448 mod 512 bits, and writes
  the original message length as a 64-bit little-endian integer in the last eight
  bytes, possibly spilling into one extra block. Then it runs the last block(s)
  through `processBlock()`.

## Endianness: the one dependency

MD5 is defined on little-endian 32-bit words. `processBlock()` reinterprets the
64-byte block as sixteen `uint32_t` and, on a big-endian host, byte-swaps each
word through a `LITTLEENDIAN(x)` macro before mixing it in. That decision is
gated on `__BYTE_ORDER == __BIG_ENDIAN`.

Those macros are exactly what the sibling [endian](https://github.com/Kronuz/endian)
library provides: it maps each platform's native byte-order header onto a uniform
`__BYTE_ORDER` / `__BIG_ENDIAN` set of names. `md5.cc` includes `"endian.hh"` for
them. That is the library's only external dependency, and the only reason it is
not standalone-trivial. The length encoding at the end of `processBuffer()` does
not rely on it: it writes the eight length bytes one at a time, low byte first,
so it is little-endian by construction on any host.

## Finalizing without consuming

`getHash()` has to pad and run the final block, which mutates the running state.
But a caller may want to keep adding bytes after reading an intermediate digest.
So `getHash(unsigned char[16])` saves the four state words, calls
`processBuffer()`, extracts the 16 digest bytes (each word written low byte
first), and then restores the saved words. The accumulated state is left exactly
as it was, so `getHash()` is repeatable and `add()` can continue afterward. The
hex form, `getHash()` returning a `std::string`, is a thin wrapper that formats
those 16 bytes as 32 lowercase hex characters.

## Why this shape

The implementation is the classic compact MD5 (Stephan Brumme's), chosen because
it is small, fast, and dependency-free except for the byte-order macros. In
Xapiand it backs content fingerprints and checksums for content-addressing, where
the value is a cheap, stable, fixed-size digest of a buffer, not a security
guarantee. The streaming `add()` / `getHash()` split is what lets it digest data
that never fully lands in memory. Keeping the block transform, padding, and length
encoding in one translation unit keeps the whole algorithm in one place, with the
only seam being the endian macros pulled from the sibling library.

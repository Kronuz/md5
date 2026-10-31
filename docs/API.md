# md5 API reference

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

## Caveats

- MD5 is cryptographically broken: collisions are cheap to produce. Use it only
  where an existing format requires MD5, never for integrity against an adversary.
- Input buffers need no alignment: `processBlock()` loads each 32-bit word with
  `memcpy`. `test_unaligned_input` covers offsets 0 to 15, one-shot and streamed.

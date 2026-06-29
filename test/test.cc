// Smoke test for the standalone md5 library.
//
// Exercises MD5: the RFC 1321 test-suite vectors via the string and (void*,
// size) call operators, the streaming add()/getHash() path, that getHash() is
// repeatable (it does not consume the state), reset() between hashes, embedded
// NULs (it is size-based, not NUL-terminated), and a long input that spans many
// 64-byte blocks.
//
// Build via CMake: cmake -B build && cmake --build build && ctest --test-dir build
#include <cassert>
#include <cstdio>
#include <string>
#include <string_view>

#include "md5.h"


// ---------------------------------------------------------------------------
// The RFC 1321 test suite: the canonical (input, digest) vectors. If these
// match, the core transform, padding, and length encoding are all correct.
// ---------------------------------------------------------------------------

static void test_rfc1321_vectors() {
	MD5 md5;
	assert(md5(std::string_view("")) == "d41d8cd98f00b204e9800998ecf8427e");
	assert(md5(std::string_view("a")) == "0cc175b9c0f1b6a831c399e269772661");
	assert(md5(std::string_view("abc")) == "900150983cd24fb0d6963f7d28e17f72");
	assert(md5(std::string_view("message digest")) == "f96b697d7cb7938d525a2f31aaf161d0");
	assert(md5(std::string_view("abcdefghijklmnopqrstuvwxyz")) == "c3fcd3d76192e4007dfb496cca67e13b");
	assert(md5(std::string_view("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"))
	       == "d174ab98d277d9f5a5611c2c9f419d9f");
	assert(md5(std::string_view("12345678901234567890123456789012345678901234567890123456789012345678901234567890"))
	       == "57edf4a22be3c955ac49da2e2107b67a");

	std::printf("md5 RFC 1321 vectors OK: empty, a, abc, ... the full suite\n");
}


// ---------------------------------------------------------------------------
// The (void*, size) overload agrees with the string_view one, and is what you
// reach for with arbitrary (possibly non-string) data.
// ---------------------------------------------------------------------------

static void test_void_size_overload() {
	MD5 md5;
	const char* data = "abc";
	assert(md5(data, 3) == "900150983cd24fb0d6963f7d28e17f72");

	// "How are you" truncated to 11 bytes (the header's own usage example).
	assert(md5("How are you", 11) == md5(std::string_view("How are you")));

	std::printf("md5 (void*, size) OK: agrees with string_view, honors the byte count\n");
}


// ---------------------------------------------------------------------------
// Streaming: feeding the message in chunks via add() must give the same digest
// as hashing it in one shot, including chunk boundaries that fall mid-block.
// ---------------------------------------------------------------------------

static void test_streaming() {
	const std::string_view text("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
	const std::string expected = "d174ab98d277d9f5a5611c2c9f419d9f";

	MD5 md5;
	// Feed it one byte at a time: this crosses the 64-byte block boundary at an
	// awkward offset, exercising the internal buffer's carry logic.
	for (char ch : text) {
		md5.add(&ch, 1);
	}
	assert(md5.getHash() == expected);

	std::printf("md5 streaming OK: byte-at-a-time add() matches the one-shot digest\n");
}


// ---------------------------------------------------------------------------
// getHash() is repeatable: calling it twice on the same accumulated state must
// return the same digest (it saves and restores the hash around finalization).
// ---------------------------------------------------------------------------

static void test_gethash_repeatable() {
	MD5 md5;
	md5.add("abc", 3);
	std::string first = md5.getHash();
	std::string second = md5.getHash();
	assert(first == second);
	assert(first == "900150983cd24fb0d6963f7d28e17f72");

	std::printf("md5 getHash() repeatable OK: finalizing does not consume the state\n");
}


// ---------------------------------------------------------------------------
// reset() returns the object to its initial state, so one MD5 instance can hash
// several independent inputs. The call operators reset() for you.
// ---------------------------------------------------------------------------

static void test_reset_reuse() {
	MD5 md5;
	assert(md5(std::string_view("abc")) == "900150983cd24fb0d6963f7d28e17f72");
	// The same object, reused: the operator() resets it before hashing.
	assert(md5(std::string_view("")) == "d41d8cd98f00b204e9800998ecf8427e");
	assert(md5(std::string_view("a")) == "0cc175b9c0f1b6a831c399e269772661");

	std::printf("md5 reset/reuse OK: one instance hashes independent inputs\n");
}


// ---------------------------------------------------------------------------
// Embedded NULs: md5 is size-based, so a \0 in the middle is hashed, not treated
// as a terminator. The digest of "a\0b" differs from the digest of "a".
// ---------------------------------------------------------------------------

static void test_embedded_nul() {
	const char buf[] = {'a', '\0', 'b'};
	MD5 md5;
	std::string with_nul = md5(buf, sizeof(buf));
	std::string just_a = md5(std::string_view("a"));
	assert(with_nul != just_a);
	// std::string carrying an interior NUL keeps its full length.
	std::string s("a\0b", 3);
	assert(md5(s.data(), s.size()) == with_nul);

	std::printf("md5 embedded NUL OK: \\0 is hashed, length preserved\n");
}


// ---------------------------------------------------------------------------
// A long input spanning many blocks, fed both in one shot and in two halves, to
// confirm the multi-block path and the length encoding hold up past 64 bytes.
// ---------------------------------------------------------------------------

static void test_long_input() {
	std::string big(1000, 'a');  // 1000 'a' characters

	MD5 one_shot;
	std::string whole = one_shot(big.data(), big.size());

	MD5 halves;
	halves.add(big.data(), 500);
	halves.add(big.data() + 500, 500);
	assert(halves.getHash() == whole);

	// 1000 'a' has a well-known MD5 digest.
	assert(whole == "cabe45dcc9ae5b66ba86600cca6b8ba8");

	std::printf("md5 long input OK: 1000 bytes, split feed matches one-shot\n");
}


int main() {
	test_rfc1321_vectors();
	test_void_size_overload();
	test_streaming();
	test_gethash_repeatable();
	test_reset_reuse();
	test_embedded_nul();
	test_long_input();
	std::printf("all md5 tests passed\n");
	return 0;
}

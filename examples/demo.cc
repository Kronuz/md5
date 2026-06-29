// A runnable tour of md5.
//
// Build (when this repo is the top-level project):
//   cmake -B build && cmake --build build && ./build/md5_demo
//
// The one idea worth taking away: MD5 turns an arbitrary byte buffer into a
// fixed 16-byte digest, printed here as 32 lowercase hex characters. It is used
// as a fast content fingerprint / checksum for content-addressing, not as a
// security primitive. You can hash a whole buffer in one call, or stream it in
// with add() and finalize with getHash().
#include <cstdio>
#include <string>
#include <string_view>

#include "md5.h"

static void rule(const char* title) {
	std::printf("\n\033[1m── %s ──\033[0m\n", title);
}

int main() {
	std::puts("md5 demo");

	// --- 1. hash a couple of strings -----------------------------------------
	rule("a string becomes a 32-char hex digest");
	MD5 md5;
	// The call operator resets, hashes, and returns the hex digest in one go.
	std::printf("  %-14s -> %s\n", "\"\" (empty)", md5(std::string_view("")).c_str());
	std::printf("  %-14s -> %s\n", "\"abc\"", md5(std::string_view("abc")).c_str());
	std::printf("  %-14s -> %s\n", "\"hello, world\"",
	            md5(std::string_view("hello, world")).c_str());

	// --- 2. arbitrary bytes, by pointer and size -----------------------------
	rule("any byte buffer, by (pointer, size)");
	// It is size-based, so an embedded NUL is part of the content, not a
	// terminator: "a\0b" hashes all three bytes.
	const char raw[] = {'a', '\0', 'b'};
	std::printf("  %-14s -> %s\n", "\"a\\0b\" (3 b)", md5(raw, sizeof(raw)).c_str());

	// --- 3. streaming: feed it in chunks -------------------------------------
	rule("stream the input with add(), finalize with getHash()");
	// Same digest as hashing the whole thing at once, but you never need the
	// full message in memory.
	MD5 stream;
	stream.add("message ", 8);
	stream.add("digest", 6);
	std::printf("  add(\"message \") + add(\"digest\") -> %s\n", stream.getHash().c_str());
	std::printf("  md5(\"message digest\")            -> %s\n",
	            md5(std::string_view("message digest")).c_str());

	std::puts("\ndone.");
	return 0;
}

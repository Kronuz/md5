// MD5 against OpenSSL's EVP_md5, an implementation with no code in common with this one.
// Built only when CMake finds OpenSSL. The checks run in every build type.
//
// Covered: every length from 0 to 1100 and random lengths up to 70000 at every
// misalignment; one-shot, string_view, reuse of one object, raw getHash(); streaming in
// random chunks with getHash() at random cut points (it must leave the stream appendable);
// reset(). Two streams pass 2 MiB and 512 MiB, where the 64-bit message length first needs
// its fourth and fifth byte.
#include "md5.h"

#include <openssl/evp.h>

#include <algorithm>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <string_view>
#include <vector>

static int failures = 0;
#define CHECK(c) \
	do { \
		if (!(c)) { \
			if (++failures <= 20) std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
		} \
	} while (0)

static std::string hex(const unsigned char* p, std::size_t n) {
	static const char digits[] = "0123456789abcdef";
	std::string r(2 * n, '\0');
	for (std::size_t i = 0; i < n; ++i) {
		r[2 * i] = digits[p[i] >> 4];
		r[2 * i + 1] = digits[p[i] & 15];
	}
	return r;
}

// One streaming OpenSSL digest, so a prefix can be finalized on a copy.
struct Evp {
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	bool ok = ctx && EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) == 1;
	~Evp() { EVP_MD_CTX_free(ctx); }
	Evp(const Evp&) = delete;
	Evp& operator=(const Evp&) = delete;
	Evp() = default;
	void update(const void* p, std::size_t n) { EVP_DigestUpdate(ctx, p, n); }
	void raw(unsigned char out[16]) const {  // digest of what was added so far
		EVP_MD_CTX* copy = EVP_MD_CTX_new();
		unsigned len = 0;
		EVP_MD_CTX_copy_ex(copy, ctx);
		EVP_DigestFinal_ex(copy, out, &len);
		EVP_MD_CTX_free(copy);
	}
};

static std::string evp_hex(const void* p, std::size_t n) {
	Evp e;
	e.update(p, n);
	unsigned char out[16];
	e.raw(out);
	return hex(out, 16);
}

static void check_lengths(const unsigned char* buf, std::size_t len, std::mt19937_64& g) {
	const unsigned char* p = buf + len % 16;  // every misalignment
	const std::string want = evp_hex(p, len);
	MD5 h;
	const std::string got = h(p, len);
	CHECK(got == want);
	CHECK(got.size() == 2 * MD5::HashBytes);
	CHECK(h(p, len) == want);  // operator() restarts the object
	CHECK(h(std::string_view(reinterpret_cast<const char*>(p), len)) == want);

	// Streaming in random chunks, with getHash() (hex and raw) at random cut points.
	MD5 s;
	Evp e;
	std::size_t pos = 0;
	while (pos < len) {
		std::size_t chunk = std::min<std::size_t>(1 + g() % 130, len - pos);
		s.add(p + pos, chunk);
		e.update(p + pos, chunk);
		pos += chunk;
		if (g() % 5 == 0) {
			unsigned char expected[16], raw[MD5::HashBytes];
			e.raw(expected);
			CHECK(s.getHash() == hex(expected, 16));
			s.getHash(raw);
			CHECK(std::memcmp(raw, expected, 16) == 0);
		}
	}
	CHECK(s.getHash() == want);
	s.reset();
	s.add(p, len);
	CHECK(s.getHash() == want);
}

// A stream of `total` bytes: the same 1 MiB block of noise, fed in odd-sized pieces.
static void check_stream(const std::vector<unsigned char>& noise, std::uint64_t total) {
	MD5 s;
	Evp e;
	std::uint64_t done = 0;
	std::size_t step = 1;
	while (done < total) {
		std::size_t n = std::min<std::uint64_t>({noise.size() - (done % 7), total - done, step});
		s.add(noise.data() + done % 7, n);
		e.update(noise.data() + done % 7, n);
		done += n;
		step = step * 5 / 4 + 1;
		if (step > (1u << 20) - 7) step = 1;
	}
	unsigned char expected[16];
	e.raw(expected);
	CHECK(s.getHash() == hex(expected, 16));
}

int main() {
	Evp probe;
	if (!probe.ok) {  // MD5 is not available to OpenSSL (a FIPS-only build)
		std::puts("md5 vs OpenSSL: EVP_md5 is unavailable, skipped");
		return 77;
	}
	std::mt19937_64 g(2026);
	std::vector<unsigned char> buf(70000 + 16);
	for (auto& b : buf) b = static_cast<unsigned char>(g());

	std::size_t cases = 0;
	for (std::size_t len = 0; len <= 1100; ++len, ++cases) check_lengths(buf.data(), len, g);
	for (int i = 0; i < 400; ++i, ++cases) check_lengths(buf.data(), g() % 70000, g);
	for (std::size_t len : {8191u, 8192u, 8193u, 16384u, 65535u, 65536u, 69999u}) {
		check_lengths(buf.data(), len, g);
		++cases;
	}

	std::vector<unsigned char> noise((1u << 20) + 7);
	for (auto& b : noise) b = static_cast<unsigned char>(g());
	check_stream(noise, (2u << 20) + 13);      // length bits past 2^24
	check_stream(noise, (512ull << 20) + 21);  // length bits past 2^32

	if (failures) {
		std::fprintf(stderr, "md5 vs OpenSSL: %d failures\n", failures);
		return 1;
	}
	std::printf("md5 matches OpenSSL EVP: %zu lengths and two long streams\n", cases);
	return 0;
}

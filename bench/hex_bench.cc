// MD5 string API: ns per call, input to 32-character hex string, on one thread.
// Best of N windows; the first input byte changes every call so nothing folds.
// With MD5_BENCH_OPENSSL the table adds OpenSSL EVP: the raw 16-byte digest with the algorithm
// fetched once, the same plus the hex conversion, and the raw digest through EVP_md5().
// Each row first checks that the hex strings agree.
// Usage: md5_hex_bench [windows]
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "md5.h"

#ifdef MD5_BENCH_OPENSSL
#include <openssl/evp.h>
#endif

using clk = std::chrono::steady_clock;
static volatile unsigned long long sink;

// Calls f(i) in batches until one batch lasts target_ms, then keeps the fastest of `windows`.
template <class F>
static double best_ns(int windows, double target_ms, F f) {
	long long batch = 16;
	for (;;) {
		auto t0 = clk::now();
		for (long long i = 0; i < batch; ++i) f(i);
		if (std::chrono::duration<double, std::milli>(clk::now() - t0).count() > target_ms) break;
		batch *= 2;
	}
	double best = 1e300;
	for (int w = 0; w < windows; ++w) {
		auto t0 = clk::now();
		for (long long i = 0; i < batch; ++i) f(i);
		best = std::min(best, std::chrono::duration<double, std::nano>(clk::now() - t0).count() / batch);
	}
	return best;
}

#ifdef MD5_BENCH_OPENSSL
static std::string hex(const unsigned char* p, std::size_t n) {
	static const char digits[] = "0123456789abcdef";
	std::string r(2 * n, '\0');
	char* out = r.data();
	for (std::size_t i = 0; i < n; ++i) {
		*out++ = digits[p[i] >> 4];
		*out++ = digits[p[i] & 15];
	}
	return r;
}
#endif

int main(int argc, char** argv) {
	const int windows = argc > 1 ? std::atoi(argv[1]) : 7;
	std::vector<unsigned char> data(1 << 20, 'x');

#ifdef MD5_BENCH_OPENSSL
	// OpenSSL 3 looks the algorithm up on every EVP_DigestInit_ex(ctx, EVP_md5()). The "fetched"
	// columns look it up once; the "EVP_md5()" column is the call most code writes.
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	EVP_MD* md = EVP_MD_fetch(nullptr, "MD5", nullptr);
#else
	const EVP_MD* md = EVP_md5();
#endif
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	auto evp = [&](const EVP_MD* m, std::size_t n, unsigned char out[16]) {
		unsigned len;
		EVP_DigestInit_ex(ctx, m, nullptr);
		EVP_DigestUpdate(ctx, data.data(), n);
		EVP_DigestFinal_ex(ctx, out, &len);
	};
	std::printf("%8s | %12s | %12s %12s %12s | %s\n", "bytes", "MD5 hex", "EVP fetched", "+ hex", "EVP_md5()", "MD5 / (EVP fetched + hex)");
#else
	std::printf("%8s | %12s\n", "bytes", "MD5 hex");
#endif

	for (std::size_t n : {16, 64, 1024, 65536, 1048576}) {
		const double target_ms = n >= 65536 ? 40 : 20;
		MD5 h;
		const double ours = best_ns(windows, target_ms, [&](long long i) {
			data[0] = static_cast<unsigned char>(i);
			sink = static_cast<unsigned char>(h(data.data(), n)[0]);
		});
#ifdef MD5_BENCH_OPENSSL
		{
			unsigned char raw[16];
			evp(md, n, raw);
			if (h(data.data(), n) != hex(raw, 16)) {
				std::fprintf(stderr, "MD5 and OpenSSL disagree at %zu bytes\n", n);
				return 1;
			}
		}
		const double raw_ns = best_ns(windows, target_ms, [&](long long i) {
			unsigned char raw[16];
			data[0] = static_cast<unsigned char>(i);
			evp(md, n, raw);
			sink = raw[0];
		});
		const double hex_ns = best_ns(windows, target_ms, [&](long long i) {
			unsigned char raw[16];
			data[0] = static_cast<unsigned char>(i);
			evp(md, n, raw);
			sink = static_cast<unsigned char>(hex(raw, 16)[0]);
		});
		const double implicit_ns = best_ns(windows, target_ms, [&](long long i) {
			unsigned char raw[16];
			data[0] = static_cast<unsigned char>(i);
			evp(EVP_md5(), n, raw);
			sink = raw[0];
		});
		std::printf("%8zu | %9.1f ns | %9.1f ns %9.1f ns %9.1f ns | %.2f\n", n, ours, raw_ns, hex_ns, implicit_ns, ours / hex_ns);
#else
		std::printf("%8zu | %9.1f ns\n", n, ours);
#endif
	}

#ifdef MD5_BENCH_OPENSSL
	EVP_MD_CTX_free(ctx);
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	EVP_MD_free(md);
#endif
#endif
}

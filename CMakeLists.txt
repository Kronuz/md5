cmake_minimum_required(VERSION 3.14)
project(md5 LANGUAGES CXX)

include(FetchContent)

# --- Dependency: endian ---
# md5.cc includes "endian.hh" for the platform byte-order macros (__BYTE_ORDER /
# __BIG_ENDIAN), used to decide whether the 32-bit words need byte-swapping. That
# header lives in the sibling endian library, pulled in here at a pinned commit
# so the include resolves.
FetchContent_Declare(
  endian
  GIT_REPOSITORY https://github.com/Kronuz/endian.git
  GIT_TAG        a24942eabaad0b1c8e0b3404b4ae5a10b624b4c3
)
FetchContent_MakeAvailable(endian)

# --- Library target ---
# md5 is not header-only: md5.cc carries the whole digest implementation, so it
# must be compiled and linked. The header alone (md5.h) only declares the MD5
# class.
add_library(md5 md5.cc)
target_include_directories(md5 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(md5 PUBLIC cxx_std_20)

# Link endian so "endian.hh" is on the include path. PUBLIC because md5.cc uses
# it; consumers don't need endian.hh themselves, but linking it PUBLIC keeps the
# include path consistent and harmless.
target_link_libraries(md5 PUBLIC endian::endian)

# Alias so consumers can link `md5::md5` whether they add this via
# add_subdirectory or FetchContent.
add_library(md5::md5 ALIAS md5)

# Only build the test and demo when this repo is the top-level project, so
# consumers (e.g. via FetchContent) don't build them.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_executable(md5_test test/test.cc)
  target_link_libraries(md5_test PRIVATE md5)
  add_test(NAME md5 COMMAND md5_test)

  # Against OpenSSL's EVP_md5 (an independent implementation) when OpenSSL is installed.
  # The test exits 77 (skipped) where OpenSSL has no MD5.
  find_package(OpenSSL QUIET COMPONENTS Crypto)
  if(OpenSSL_FOUND)
    add_executable(md5_openssl_test test/openssl.cc)
    target_link_libraries(md5_openssl_test PRIVATE md5 OpenSSL::Crypto)
    add_test(NAME md5_openssl COMMAND md5_openssl_test)
    set_tests_properties(md5_openssl PROPERTIES SKIP_RETURN_CODE 77)
  endif()

  # Legacy MD5 string API timing, with OpenSSL EVP columns when OpenSSL is found:
  # cmake -B build -DCMAKE_BUILD_TYPE=Release -DMD5_BUILD_BENCH=ON
  option(MD5_BUILD_BENCH "Build the hex-string benchmark" OFF)
  if(MD5_BUILD_BENCH)
    add_executable(md5_hex_bench bench/hex_bench.cc)
    target_link_libraries(md5_hex_bench PRIVATE md5)
    if(OpenSSL_FOUND)
      target_link_libraries(md5_hex_bench PRIVATE OpenSSL::Crypto)
      target_compile_definitions(md5_hex_bench PRIVATE MD5_BENCH_OPENSSL)
    else()
      message(STATUS "md5_hex_bench: OpenSSL not found, the EVP columns are skipped")
    endif()
  endif()

  # A runnable tour of the library (not a test): build & run ./build/md5_demo
  add_executable(md5_demo examples/demo.cc)
  target_link_libraries(md5_demo PRIVATE md5)
endif()

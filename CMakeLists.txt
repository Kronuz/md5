cmake_minimum_required(VERSION 3.14)
project(md5 LANGUAGES CXX)

include(FetchContent)

# --- Dependency: endian ---
# md5.cc includes "endian.hh" for the platform byte-order macros (__BYTE_ORDER /
# __BIG_ENDIAN), used to decide whether the 32-bit words need byte-swapping. That
# header lives in the sibling endian library, pulled in here so the include
# resolves. We track its tip with GIT_TAG main, like the rest of the family.
FetchContent_Declare(
  endian
  GIT_REPOSITORY https://github.com/Kronuz/endian.git
  GIT_TAG        f6269f09da90bb3713018792003f01a67749b00c
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

  # Legacy MD5 string API timing: cmake -B build -DMD5_BUILD_BENCH=ON
  option(MD5_BUILD_BENCH "Build the hex-string benchmark" OFF)
  if(MD5_BUILD_BENCH)
    add_executable(md5_hex_bench bench/hex_bench.cc)
    target_link_libraries(md5_hex_bench PRIVATE md5)
  endif()

  # A runnable tour of the library (not a test): build & run ./build/md5_demo
  add_executable(md5_demo examples/demo.cc)
  target_link_libraries(md5_demo PRIVATE md5)
endif()

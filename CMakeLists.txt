cmake_minimum_required(VERSION 3.25)
project(feedhandler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FH_BUILD_TESTS "Build tests" ON)
option(FH_BUILD_BENCH "Build benchmarks" ON)
option(FH_WERROR "Treat warnings as errors" OFF)
option(FH_NATIVE "Compile with -march=native (results are NOT portable)" OFF)
set(FH_SANITIZE "" CACHE STRING "Sanitizers, comma-separated: address,undefined | thread")

# ---- common flags: everything links this, so sanitizer flags are consistent ----
add_library(fh_options INTERFACE)
target_compile_options(fh_options INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual)
if(FH_WERROR)
  target_compile_options(fh_options INTERFACE -Werror)
endif()
if(FH_NATIVE)
  target_compile_options(fh_options INTERFACE -march=native)
endif()
if(FH_SANITIZE)
  target_compile_options(fh_options INTERFACE
      -fsanitize=${FH_SANITIZE} -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(fh_options INTERFACE -fsanitize=${FH_SANITIZE})
endif()

# ---- library (header-only for now) ----
add_library(fh INTERFACE)
add_library(fh::fh ALIAS fh)
target_include_directories(fh INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(fh INTERFACE cxx_std_20)
target_link_libraries(fh INTERFACE fh_options)

find_package(Threads REQUIRED)

if(FH_BUILD_TESTS)
  enable_testing()
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
endif()

if(FH_BUILD_BENCH)
  find_package(benchmark CONFIG REQUIRED)
  add_subdirectory(bench)
endif()

cmake_minimum_required(VERSION 3.25)
project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# To pin g++-14 in CMakeLists (optional; using a toolchain or presets is preferred)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.0)
    message(WARNING "GCC 14 or newer is recommended for full C++23 support (std::expected, std::println).")
endif()

find_package(ftxui CONFIG REQUIRED)

# Option to enable sanitizers
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined,leak -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined,leak)
endif()

enable_testing()

add_subdirectory(lib)

add_executable(orderbook
    src/main.cpp
)

target_include_directories(orderbook PRIVATE src)
target_link_libraries(orderbook PRIVATE
    orderbook::lib
    ftxui::screen
    ftxui::dom
    ftxui::component
)
target_compile_options(orderbook PRIVATE -Wall -Wextra -Wpedantic)

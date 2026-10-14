cmake_minimum_required(VERSION 3.25)
project(feedhandler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(quill CONFIG REQUIRED)

# Опция для включения санитайзеров
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined,leak -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined,leak)
endif()

add_executable(feedhandler
    src/main.cpp
    src/gateway/binance_listener_service.cpp
    src/writer/filewriter.hpp
    src/orderbook/order_book.hpp
)

target_include_directories(feedhandler PRIVATE src)
target_link_libraries(feedhandler PRIVATE OpenSSL::SSL OpenSSL::Crypto quill::quill)
target_compile_options(feedhandler PRIVATE -Wall -Wextra -Wpedantic)

add_library(domain INTERFACE)
target_include_directories(domain INTERFACE src)

enable_testing()
find_package(Catch2 CONFIG REQUIRED)
add_executable(binance_convert_test tests/binance_convert_test.cpp)
target_link_libraries(binance_convert_test PRIVATE domain Catch2::Catch2WithMain)
target_compile_options(binance_convert_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME binance_convert_test COMMAND binance_convert_test)
cmake_minimum_required(VERSION 3.25)
project(feedhandler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

# Опция для включения санитайзеров
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined,leak -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined,leak)
endif()

add_executable(feedhandler src/main.cpp src/listener/service.cpp)

target_include_directories(feedhandler PRIVATE src)
target_link_libraries(feedhandler PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_compile_options(feedhandler PRIVATE -Wall -Wextra -Wpedantic)

add_library(domain INTERFACE)
target_include_directories(domain INTERFACE src)

enable_testing()
find_package(Catch2 CONFIG REQUIRED)
# add_executable(decimal_test tests/decimal_test.cpp)
# target_link_libraries(decimal_test PRIVATE domain Catch2::Catch2WithMain)
# target_compile_options(decimal_test PRIVATE -Wall -Wextra -Wpedantic)
# add_test(NAME decimal_test COMMAND decimal_test)
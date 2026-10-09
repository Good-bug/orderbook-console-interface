#include <version>
#if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202202L
#error \
    "std::expected недоступен: нужен GCC 12+/Clang 16+ (с подходящей stdlib)/MSVC 19.33+ и -std=c++23"
#endif

#include <iostream>
#include <string>
#include <unordered_map>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "listener/service.h"

int main() {
    const std::string host = "stream.binance.com";
    const std::string port = "9443";
    std::unordered_map<std::string, unsigned int> target{{"btcusdt", 100}};

    try {
        BinanceListenerService service{host, port, std::move(target)};
        service.init();
        service.snifer();
    } catch (const boost::beast::system_error& se) {
        if (se.code() == boost::beast::websocket::error::closed) {
            std::cout << "connection closed cleanly\n";
            return 0;
        }
        std::cerr << "system error: " << se.code().message() << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
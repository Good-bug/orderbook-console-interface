#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class BinanceListenerService {
public:
    BinanceListenerService(std::string host, std::string port,
                           std::unordered_map<std::string, unsigned int> target);

    BinanceListenerService(std::string host, std::string port,
                           std::string target_path);

    void init();
    void snifer();
    void run();
    void stop();

private:
    std::string build_target_path() const;

    std::string const host;
    std::string const port;
    std::unordered_map<std::string, unsigned int> target;
    std::string target_path;

    // Order matters: ioc and ctx must outlive ws!
    net::io_context ioc;
    ssl::context ctx{ssl::context::tls_client};
    std::optional<websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ws;
};
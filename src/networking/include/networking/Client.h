//
// Created by michael on 01.11.2020.
//

#ifndef CLIENT_SERVER_SRC_NETWORKING_INCLUDE_NETWORKING_CLIENT_H
#define CLIENT_SERVER_SRC_NETWORKING_INCLUDE_NETWORKING_CLIENT_H

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>

class Client {
  const std::string host;
  const std::string port;

  boost::asio::io_context ioc;
  boost::asio::ip::tcp::resolver resolver;
  boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
  boost::asio::steady_timer waiter;

  boost::circular_buffer<std::string> buffer;
  std::optional<std::function<void(std::string)>> callback;

  void connect(boost::asio::yield_context yield);
  void reader(boost::asio::yield_context yield);
  void writer(boost::asio::yield_context yield);

public:
  Client(std::string_view host_, std::string_view port_);
  void subscribe(std::function<void(std::string)> f);
  void send(std::string msg);
  ~Client();
};

#endif //CLIENT_SERVER_SRC_NETWORKING_INCLUDE_NETWORKING_CLIENT_H

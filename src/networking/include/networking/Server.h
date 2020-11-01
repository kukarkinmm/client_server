//
// Created by michael on 01.11.2020.
//

#ifndef CLIENT_SERVER_SRC_NETWORKING_INCLUDE_NETWORKING_SERVER_H
#define CLIENT_SERVER_SRC_NETWORKING_INCLUDE_NETWORKING_SERVER_H

#include <set>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>

class Server {
  const boost::asio::ip::tcp::endpoint endpoint;

  boost::asio::io_context ioc;
  boost::asio::steady_timer waiter;
  std::set<std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>>> websockets;

  boost::circular_buffer<std::string> buffer;
  std::optional<std::function<void(std::string)>> callback;

  boost::asio::signal_set signals;

  void listen(boost::asio::yield_context yield);
  void connect(boost::beast::websocket::stream<boost::beast::tcp_stream> ws, boost::asio::yield_context yield);
  void reader(std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>> ws,
              boost::asio::yield_context yield);
  void broadcaster(boost::asio::yield_context yield);

public:
  Server(std::string_view host_, std::string_view port_);
  void subscribe(std::function<void(std::string)> f);
  void broadcast(std::string msg);
  [[nodiscard]] bool stopped() const;
  ~Server();
};

#endif //CLIENT_SERVER_SRC_NETWORKING_INCLUDE_NETWORKING_SERVER_H

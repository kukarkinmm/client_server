//
// Created by michael on 01.11.2020.
//

#include "networking/Server.h"

Server::Server(std::string_view host_, std::string_view port_)
    : endpoint{boost::asio::ip::make_address(host_), boost::lexical_cast<unsigned short>(port_)},
      ioc{},
      waiter{ioc},
      websockets{},
      buffer{100},
      signals{ioc, SIGTERM, SIGHUP} {
  boost::asio::spawn(ioc, [this](auto yield) {
    listen(yield);
  });
  boost::asio::spawn(ioc, [this](auto yield) {
    broadcaster(yield);
  });
  boost::asio::post([this] { ioc.run(); });
  signals.async_wait([&](auto &ec, int signal_number){
    ioc.stop();
  });
}

void Server::listen(boost::asio::yield_context yield) {
  boost::beast::error_code ec;
  boost::asio::ip::tcp::acceptor acceptor{ioc};
  acceptor.open(endpoint.protocol(), ec);
  acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
  acceptor.bind(endpoint, ec);
  acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    acceptor.close(ec);
    return;
  }
  while (acceptor.is_open()) {
    boost::asio::ip::tcp::socket socket{ioc};
    acceptor.async_accept(socket, yield[ec]);
    boost::asio::spawn(ioc, [this, socket = std::move(socket)](auto yield) mutable {
      connect(boost::beast::websocket::stream<boost::beast::tcp_stream>{std::move(socket)}, yield);
    });
  }
}

void Server::connect(boost::beast::websocket::stream<boost::beast::tcp_stream> ws, boost::asio::yield_context yield) {
  boost::beast::error_code ec;
  ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type &res) {
    res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-coro");
  }));
  ws.async_accept(yield[ec]);
  if (ec) {
    ws.async_close(boost::beast::websocket::close_reason("accept error"), yield[ec]);
    return;
  }
  auto it =
      websockets.insert(std::make_shared<boost::beast::websocket::stream<boost::beast::tcp_stream>>(std::move(ws))).first;
  boost::asio::spawn(ioc, [this, &ws = *it](auto yield) {
    reader(ws, yield);
  });
  waiter.cancel();
}

void Server::reader(std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>> ws,
                    boost::asio::yield_context yield) {
  boost::beast::error_code ec;
  boost::beast::flat_buffer b;
  while (ws->is_open()) {
    auto i = ws->async_read(b, yield[ec]);
    if (ec == boost::beast::websocket::error::closed)
      break;
    if (ec) {
      ws->async_close(boost::beast::websocket::close_reason("read error"), yield[ec]);
      break;
    }
    std::string msg{static_cast<char *>(b.data().data()), b.size()};
    b.consume(i);
    if (callback) {
      boost::asio::post(ioc, [this, msg = std::move(msg)]() mutable {
        (*callback)(std::move(msg));
      });
    }
  }
  websockets.erase(ws);
}

void Server::broadcaster(boost::asio::yield_context yield) {
  boost::beast::error_code ec;
  while (true) {
    if (websockets.empty() || buffer.empty()) {
      waiter.expires_from_now(std::chrono::steady_clock::duration::max());
      waiter.async_wait(yield[ec]);
      continue;
    }
    auto str = std::move(buffer.back());
    buffer.pop_back();
    for (auto &ws : websockets) {
      if (ws->is_open()) {
        ws->async_write(boost::asio::buffer(str), yield[ec]);
      }
    }
  }
}

void Server::subscribe(std::function<void(std::string)> f) {
  boost::asio::post(ioc, [this, f = std::move(f)] {
    callback = f;
  });
}

void Server::broadcast(std::string msg) {
  boost::asio::post(ioc, [this, msg = std::move(msg)]() mutable {
    buffer.push_front(std::move(msg));
    waiter.cancel();
  });
}

bool Server::stopped() const {
  return ioc.stopped();
}

Server::~Server() {
  ioc.stop();
}

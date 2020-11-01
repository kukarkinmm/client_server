//
// Created by michael on 01.11.2020.
//

#include "networking/Client.h"

Client::Client(std::string_view host_, std::string_view port_)
  : host(host_), port(port_), ioc{}, resolver{ioc}, ws{ioc}, waiter{ioc}, buffer{100} {
  boost::asio::spawn(ioc, [this](auto yield) {
    connect(yield);
  });
  boost::asio::post([this] {
    ioc.run();
  });
}

void Client::connect(boost::asio::yield_context yield) {
  boost::beast::error_code ec;
  while (true) {
    auto const results = resolver.async_resolve(host, port, yield[ec]);
    if (ec)
      continue;
    auto ep = boost::beast::get_lowest_layer(ws).async_connect(results, yield[ec]);
    if (ec)
      continue;
    auto endpoint = host + ':' + std::to_string(ep.port());
    ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
    ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type &req) {
      req.set(boost::beast::http::field::user_agent,
              std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
    }));
    ws.async_handshake(endpoint, "/", yield[ec]);
    if (ec)
      continue;

    boost::asio::spawn(ioc, [this](auto yield) {
      reader(yield);
    });
    boost::asio::spawn(ioc, [this](auto yield) {
      writer(yield);
    });
    std::cout << "Connected to " << host << ':' << port << std::endl;
    break;
  }
}

void Client::reader(boost::asio::yield_context yield) {
  boost::beast::error_code ec;
  boost::beast::flat_buffer b;
  while (ws.is_open()) {
    auto i = ws.async_read(b, yield[ec]);
    if (ec) {
      ws.async_close(boost::beast::websocket::close_reason("read error"), yield[ec]);
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
}

void Client::writer(boost::asio::yield_context yield) {
  boost::beast::error_code ec;
  while (ws.is_open()) {
    if (buffer.empty()) {
      waiter.expires_from_now(std::chrono::steady_clock::duration::max());
      waiter.async_wait(yield[ec]);
      continue;
    }
    auto str = std::move(buffer.back());
    buffer.pop_back();
    ws.async_write(boost::asio::buffer(str), yield[ec]);
  }
}

void Client::subscribe(std::function<void(std::string)> f) {
  boost::asio::post(ioc, [this, f = std::move(f)] {
    callback = f;
  });
}

void Client::send(std::string msg) {
  boost::asio::post(ioc, [this, msg = std::move(msg)]() mutable {
    buffer.push_front(std::move(msg));
    waiter.cancel();
  });
}

Client::~Client() {
  ioc.stop();
}

//
// Created by michael on 01.11.2020.
//

#include <fstream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "networking/Client.h"

using namespace std::chrono_literals;

int main(int argc, char **argv) {
  std::string host;
  std::string port;
  std::string filename;

  boost::program_options::options_description desc{"Allowed options"};
  desc.add_options()
      ("help,h", "help message")
      ("host", boost::program_options::value<std::string>(&host)->default_value("127.0.0.1"), "host to connect")
      ("port", boost::program_options::value<std::string>(&port)->default_value("9999"), "port to connect")
      ("filename",
       boost::program_options::value<std::string>(&filename)->default_value("file.txt"),
       "file to transfer");

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, desc),
      vm
  );
  boost::program_options::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  Client client{host, port};

  std::ifstream file{filename};
  std::stringstream msg;
  std::string b;
  while (std::getline(file, b)) {
    msg << b << std::endl;
  }
  client.send(msg.str());
  file.close();
  std::this_thread::sleep_for(1s);
}

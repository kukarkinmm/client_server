#include <fstream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "networking/Server.h"
#include "networking/Client.h"

using namespace std::chrono_literals;

void daemonize() {
  pid_t pid = fork();
  if (pid < 0)
    std::exit(EXIT_FAILURE);
  if (pid > 0) {
    std::cout << "fork PID: " << pid << std::endl;
    std::exit(EXIT_SUCCESS);
  }
}

int main(int argc, char **argv) {
  std::string host;
  std::string port;

  boost::program_options::options_description desc{"Allowed options"};
  desc.add_options()
      ("help,h", "help message")
      ("host", boost::program_options::value<std::string>(&host)->default_value("127.0.0.1"), "host to connect")
      ("port", boost::program_options::value<std::string>(&port)->default_value("9999"), "port to connect");

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

  daemonize();

  std::mutex m;
  unsigned long long counter = 0;

  Server server{"127.0.0.1", "9999"};
  server.subscribe([&](const std::string &msg) {
    std::unique_lock l{m};
    std::ofstream file{"file" + std::to_string(counter++) + ".txt"};
    file << msg;
    file.close();
  });

  while (true) {
    if (server.stopped()) {
      break;
    }
    std::this_thread::sleep_for(1s);
  }

  return 0;
}

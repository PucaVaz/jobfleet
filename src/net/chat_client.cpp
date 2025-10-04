#include "../libtslog/tslog.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace {

struct Config {
  std::string host = "127.0.0.1";
  int port = 9000;
  std::string log_file = "logs/chat_client.log";
  std::string level = "info";
  // modo não interativo para testes
  int count = 0;          // número de mensagens a enviar
  std::string message;    // conteúdo base
  int delay_ms = 0;       // atraso entre envios
};

void usage(const char* prog) {
  std::cout << "Uso: " << prog << " [--host H] [--port N] [--log PATH] [--level L] [--count C --message M --delay MS]\n"
            << "Ex.: " << prog << " --host 127.0.0.1 --port 9000\n";
}

tslog::Level parse_level(const std::string& s) {
  if (s == "debug") return tslog::Level::DEBUG;
  if (s == "info") return tslog::Level::INFO;
  if (s == "warn") return tslog::Level::WARN;
  if (s == "error") return tslog::Level::ERROR;
  return tslog::Level::INFO;
}

Config parse_args(int argc, char* argv[]) {
  Config cfg;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--help" || a == "-h") {
      usage(argv[0]);
      std::exit(0);
    } else if (a == "--host" && i + 1 < argc) {
      cfg.host = argv[++i];
    } else if (a == "--port" && i + 1 < argc) {
      cfg.port = std::stoi(argv[++i]);
    } else if (a == "--log" && i + 1 < argc) {
      cfg.log_file = argv[++i];
    } else if (a == "--level" && i + 1 < argc) {
      cfg.level = argv[++i];
    } else if (a == "--count" && i + 1 < argc) {
      cfg.count = std::stoi(argv[++i]);
    } else if (a == "--message" && i + 1 < argc) {
      cfg.message = argv[++i];
    } else if (a == "--delay" && i + 1 < argc) {
      cfg.delay_ms = std::stoi(argv[++i]);
    } else {
      std::cerr << "Argumento desconhecido: " << a << "\n";
      usage(argv[0]);
      std::exit(2);
    }
  }
  return cfg;
}

volatile std::sig_atomic_t g_stop = 0;
void handle_sigint(int) { g_stop = 1; }

void reader_loop(int fd) {
  char buf[4096];
  while (!g_stop) {
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) break;
    std::string incoming(buf, buf + n);
    std::cout << incoming;
  }
}

} // namespace

int main(int argc, char* argv[]) {
  auto cfg = parse_args(argc, argv);

  tslog::Options logopt;
  logopt.file_path = cfg.log_file;
  logopt.level = parse_level(cfg.level);
  logopt.also_stdout = true;
  tslog::init(logopt);

  std::signal(SIGINT, handle_sigint);
  std::signal(SIGTERM, handle_sigint);

  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) { perror("socket"); return 1; }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(cfg.port);
  if (inet_pton(AF_INET, cfg.host.c_str(), &addr.sin_addr) <= 0) {
    std::cerr << "Endereço inválido: " << cfg.host << "\n";
    return 1;
  }

  tslog::info("Conectando a " + cfg.host + ":" + std::to_string(cfg.port));
  if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("connect");
    return 1;
  }
  tslog::info("Conectado");

  std::thread reader(reader_loop, fd);

  if (cfg.count > 0) {
    for (int i = 1; i <= cfg.count && !g_stop; ++i) {
      std::ostringstream os;
      if (!cfg.message.empty()) os << cfg.message << " ";
      os << "msg=" << i;
      std::string out = os.str() + "\n";
      send(fd, out.data(), out.size(), 0);
      tslog::debug(std::string("enviado: ") + out);
      if (cfg.delay_ms > 0) usleep(cfg.delay_ms * 1000);
    }
    shutdown(fd, SHUT_WR);
  } else {
    // modo interativo: lê stdin e envia
    std::string line;
    while (!g_stop && std::getline(std::cin, line)) {
      line += "\n";
      send(fd, line.data(), line.size(), 0);
    }
    shutdown(fd, SHUT_WR);
  }

  if (reader.joinable()) reader.join();
  close(fd);
  tslog::shutdown();
  return 0;
}


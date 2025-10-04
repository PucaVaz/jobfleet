#include "../libtslog/tslog.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Config {
  int port = 9000;
  std::string log_file = "logs/chat_server.log";
  std::string level = "info";
};

void usage(const char* prog) {
  std::cout << "Uso: " << prog << " [--port N] [--log PATH] [--level LEVEL]\n"
            << "Ex.: " << prog << " --port 9000\n";
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
    } else if (a == "--port" && i + 1 < argc) {
      cfg.port = std::stoi(argv[++i]);
    } else if (a == "--log" && i + 1 < argc) {
      cfg.log_file = argv[++i];
    } else if (a == "--level" && i + 1 < argc) {
      cfg.level = argv[++i];
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

std::string peer_to_string(int fd) {
  sockaddr_in addr{};
  socklen_t len = sizeof(addr);
  if (getpeername(fd, (sockaddr*)&addr, &len) == 0) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    std::ostringstream os;
    os << ip << ":" << ntohs(addr.sin_port);
    return os.str();
  }
  return std::string("fd=") + std::to_string(fd);
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

  int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    perror("socket");
    return 1;
  }

  int yes = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(cfg.port);

  if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  if (listen(listen_fd, 16) < 0) {
    perror("listen");
    return 1;
  }

  tslog::info("chat_server iniciado na porta " + std::to_string(cfg.port));

  std::set<int> clients;
  std::string recv_buf;

  while (!g_stop) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(listen_fd, &rfds);
    int maxfd = listen_fd;
    for (int fd : clients) {
      FD_SET(fd, &rfds);
      if (fd > maxfd) maxfd = fd;
    }

    timeval tv{1, 0}; // 1s timeout para checar sinais
    int ready = select(maxfd + 1, &rfds, nullptr, nullptr, &tv);
    if (ready < 0) {
      if (errno == EINTR) continue;
      perror("select");
      break;
    }
    if (ready == 0) {
      continue; // timeout
    }

    if (FD_ISSET(listen_fd, &rfds)) {
      sockaddr_in caddr{}; socklen_t clen = sizeof(caddr);
      int cfd = accept(listen_fd, (sockaddr*)&caddr, &clen);
      if (cfd >= 0) {
        clients.insert(cfd);
        tslog::info(std::string("conectado: ") + peer_to_string(cfd));
      }
    }

    std::vector<int> to_close;
    char buf[4096];
    for (int fd : clients) {
      if (!FD_ISSET(fd, &rfds)) continue;
      ssize_t n = recv(fd, buf, sizeof(buf), 0);
      if (n <= 0) {
        if (n == 0) {
          tslog::info(std::string("desconectado: ") + peer_to_string(fd));
        } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
          tslog::warn(std::string("erro recv de ") + peer_to_string(fd) + ": " + std::strerror(errno));
        }
        to_close.push_back(fd);
        continue;
      }
      std::string incoming(buf, buf + n);
      // Log e broadcast por linhas
      std::stringstream ss(incoming);
      std::string line;
      while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        tslog::info(std::string("msg de ") + peer_to_string(fd) + ": " + line);
        std::string out = line + "\n";
        for (int ofd : clients) {
          if (ofd == fd) continue; // não ecoar para o emissor
          send(ofd, out.data(), out.size(), 0);
        }
      }
    }

    for (int fd : to_close) {
      close(fd);
      clients.erase(fd);
    }
  }

  tslog::info("Encerrando chat_server...");
  for (int fd : clients) close(fd);
  close(listen_fd);
  tslog::shutdown();
  return 0;
}


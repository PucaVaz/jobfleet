#pragma once

#include <string>

namespace tslog {

enum class Level { DEBUG, INFO, WARN, ERROR };

struct Options {
  std::string file_path;     // ex: "logs/app.log"
  Level level = Level::INFO;
  size_t max_bytes = 0;      // 0 = sem rotação
  bool also_stdout = true;   // espelhar no stdout
};

// Inicializa o logger global (MPSC → thread única de escrita)
void init(const Options& options);

// Encerrar: flush e join da thread de escrita
void shutdown();

void set_level(Level level);

// Atalhos de log. Mensagem já formatada (sem dependências externas).
void log(Level level, const std::string& msg);
inline void debug(const std::string& m) { log(Level::DEBUG, m); }
inline void info(const std::string& m) { log(Level::INFO, m); }
inline void warn(const std::string& m) { log(Level::WARN, m); }
inline void error(const std::string& m) { log(Level::ERROR, m); }

} // namespace tslog
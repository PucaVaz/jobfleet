#include "tslog.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace tslog {

namespace {

struct Logger {
  std::mutex queue_mutex;
  std::condition_variable queue_cv;
  std::deque<std::string> message_queue;
  std::thread writer_thread;
  std::atomic<bool> shutdown_requested{false};

  Options options;
  std::atomic<Level> current_level{Level::INFO};
  std::ofstream log_file;

  void writer_loop();
  void flush_messages();
  void rotate_if_needed();
  std::string format_timestamp();
  std::string level_to_string(Level level);
  std::string get_thread_id();
};

Logger* g_logger = nullptr;

std::string Logger::format_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()) % 1000;

  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
  ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
  return ss.str();
}

std::string Logger::level_to_string(Level level) {
  switch (level) {
    case Level::DEBUG: return "DEBUG";
    case Level::INFO:  return "INFO";
    case Level::WARN:  return "WARN";
    case Level::ERROR: return "ERROR";
  }
  return "UNKNOWN";
}

std::string Logger::get_thread_id() {
  std::stringstream ss;
  ss << std::this_thread::get_id();
  return ss.str();
}

void Logger::rotate_if_needed() {
  if (options.max_bytes == 0) return;

  try {
    if (std::filesystem::exists(options.file_path)) {
      auto size = std::filesystem::file_size(options.file_path);
      if (size >= options.max_bytes) {
        log_file.close();

        std::string backup = options.file_path + ".1";
        std::filesystem::rename(options.file_path, backup);

        log_file.open(options.file_path, std::ios::app);
        if (!log_file.is_open()) {
          std::cerr << "Falha ao reabrir arquivo de log após rotação: "
                    << options.file_path << std::endl;
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Erro na rotação de log: " << e.what() << std::endl;
  }
}

void Logger::flush_messages() {
  std::vector<std::string> messages_to_write;

  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    if (message_queue.empty()) return;

    messages_to_write.reserve(message_queue.size());
    while (!message_queue.empty()) {
      messages_to_write.push_back(std::move(message_queue.front()));
      message_queue.pop_front();
    }
  }

  rotate_if_needed();

  for (const auto& msg : messages_to_write) {
    if (log_file.is_open()) {
      log_file << msg << "\n";
    }
    if (options.also_stdout) {
      std::cout << msg << "\n";
    }
  }

  if (log_file.is_open()) {
    log_file.flush();
  }
  if (options.also_stdout) {
    std::cout.flush();
  }
}

void Logger::writer_loop() {
  while (true) {
    std::unique_lock<std::mutex> lock(queue_mutex);

    queue_cv.wait_for(lock, std::chrono::milliseconds(50), [this] {
      return shutdown_requested.load() || !message_queue.empty();
    });

    if (!message_queue.empty()) {
      lock.unlock();
      flush_messages();
    } else if (shutdown_requested.load()) {
      break;
    }
  }

  // Flush final no shutdown
  flush_messages();
}

} // anonymous namespace

void init(const Options& options) {
  if (g_logger) {
    std::cerr << "Logger já foi inicializado" << std::endl;
    return;
  }

  g_logger = new Logger();
  g_logger->options = options;
  g_logger->current_level.store(options.level);

  // Cria diretório se não existir
  try {
    auto parent_path = std::filesystem::path(options.file_path).parent_path();
    if (!parent_path.empty()) {
      std::filesystem::create_directories(parent_path);
    }
  } catch (const std::exception& e) {
    std::cerr << "Falha ao criar diretório de log: " << e.what() << std::endl;
  }

  // Abre arquivo de log
  g_logger->log_file.open(options.file_path, std::ios::app);
  if (!g_logger->log_file.is_open()) {
    std::cerr << "Falha ao abrir arquivo de log: " << options.file_path << std::endl;
  }

  // Inicia thread de escrita
  g_logger->writer_thread = std::thread(&Logger::writer_loop, g_logger);
}

void shutdown() {
  if (!g_logger) return;

  g_logger->shutdown_requested.store(true);
  g_logger->queue_cv.notify_all();

  if (g_logger->writer_thread.joinable()) {
    g_logger->writer_thread.join();
  }

  if (g_logger->log_file.is_open()) {
    g_logger->log_file.close();
  }

  delete g_logger;
  g_logger = nullptr;
}

void set_level(Level level) {
  if (g_logger) {
    g_logger->current_level.store(level);
  }
}

void log(Level level, const std::string& msg) {
  if (!g_logger) return;

  if (level < g_logger->current_level.load()) return;

  try {
    pid_t pid = getpid();
    std::string tid = g_logger->get_thread_id();
    std::string timestamp = g_logger->format_timestamp();
    std::string level_str = g_logger->level_to_string(level);

    std::stringstream formatted;
    formatted << "[" << timestamp << " " << pid << " " << tid
              << " " << level_str << "] " << msg;

    {
      std::lock_guard<std::mutex> lock(g_logger->queue_mutex);
      g_logger->message_queue.push_back(formatted.str());
    }

    g_logger->queue_cv.notify_one();
  } catch (const std::exception& e) {
    std::cerr << "Erro de logging: " << e.what() << std::endl;
  }
}

} // namespace tslog
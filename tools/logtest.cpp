#include "../src/libtslog/tslog.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

struct TestConfig {
  int threads = 8;
  int lines = 100000;
  std::string output_file = "logs/test.log";
  std::string level = "info";
  bool help = false;
};

void print_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [options]\n"
            << "Options:\n"
            << "  --threads N     Number of threads (default: 8)\n"
            << "  --lines M       Lines per thread (default: 100000)\n"
            << "  --out PATH      Output file (default: logs/test.log)\n"
            << "  --level LEVEL   Log level (default: info)\n"
            << "  --help          Show this help\n";
}

TestConfig parse_args(int argc, char* argv[]) {
  TestConfig config;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "--help") {
      config.help = true;
    } else if (arg == "--threads" && i + 1 < argc) {
      config.threads = std::stoi(argv[++i]);
    } else if (arg == "--lines" && i + 1 < argc) {
      config.lines = std::stoi(argv[++i]);
    } else if (arg == "--out" && i + 1 < argc) {
      config.output_file = argv[++i];
    } else if (arg == "--level" && i + 1 < argc) {
      config.level = argv[++i];
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      config.help = true;
    }
  }

  return config;
}

tslog::Level parse_level(const std::string& level_str) {
  if (level_str == "debug") return tslog::Level::DEBUG;
  if (level_str == "info") return tslog::Level::INFO;
  if (level_str == "warn") return tslog::Level::WARN;
  if (level_str == "error") return tslog::Level::ERROR;
  return tslog::Level::INFO;
}

void worker_thread(int thread_id, int num_lines) {
  for (int i = 0; i < num_lines; i++) {
    std::string msg = "thread=" + std::to_string(thread_id) +
                      " line=" + std::to_string(i);
    tslog::info(msg);
  }
}

int main(int argc, char* argv[]) {
  TestConfig config = parse_args(argc, argv);

  if (config.help) {
    print_usage(argv[0]);
    return 0;
  }

  std::cout << "Starting log test with " << config.threads << " threads, "
            << config.lines << " lines each, output: " << config.output_file
            << std::endl;

  // Initialize logger
  tslog::Options options;
  options.file_path = config.output_file;
  options.level = parse_level(config.level);
  options.also_stdout = false; // Don't spam stdout during test

  tslog::init(options);

  auto start_time = std::chrono::high_resolution_clock::now();

  // Create and start worker threads
  std::vector<std::thread> threads;
  threads.reserve(config.threads);

  for (int i = 0; i < config.threads; i++) {
    threads.emplace_back(worker_thread, i, config.lines);
  }

  // Wait for all threads to complete
  for (auto& t : threads) {
    t.join();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  // Shutdown logger
  tslog::shutdown();

  int expected_lines = config.threads * config.lines;
  std::cout << "Test completed in " << duration.count() << "ms" << std::endl;
  std::cout << "Expected lines: " << expected_lines << std::endl;
  std::cout << "Check the log file for verification: " << config.output_file
            << std::endl;

  return 0;
}
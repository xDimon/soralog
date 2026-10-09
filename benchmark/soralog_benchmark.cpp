/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soralog_benchmark.cpp
 * @brief Throughput and latency benchmark of the event buffer and sink path.
 *
 * Scenarios:
 *  - queue:     N producers -> CircularBuffer -> 1 dedicated consumer.
 *               Isolates the buffer itself; checks per-producer FIFO order
 *               and that no element is lost or duplicated.
 *  - sink:      N producers -> SinkToFile::push (latency 1000 ms, worker
 *               thread drains the buffer) -> file.
 *  - sink-sync: same, but latency 0: every push flushes synchronously.
 *
 * For each scenario and producer count the run is repeated; throughput is
 * reported as median (min..max) over repeats, latency percentiles of a single
 * operation are computed over all operations of all repeats. Latency includes
 * the cost of reading the clock twice per operation.
 *
 * Usage:
 *   soralog_benchmark [--scenario queue|sink|sink-sync|all]
 *                     [--threads 1,2,4,8,12] [--ops N] [--repeat R]
 *                     [--capacity N] [--message-length N]
 *                     [--sink-path PATH]
 *
 * With --sink-path other than /dev/null the output file is recreated for each
 * run and its line count is verified against the number of pushed messages.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(__x86_64__) or defined(_M_X64)
#include <immintrin.h>
#endif

#include <soralog/circular_buffer.hpp>
#include <soralog/impl/sink_to_file.hpp>

namespace {

  using Clock = std::chrono::steady_clock;

  inline void cpuRelax() noexcept {
#if defined(__x86_64__) or defined(_M_X64)
    _mm_pause();
#elif defined(__aarch64__)
    asm volatile("yield" ::: "memory");
#else
    std::this_thread::yield();
#endif
  }

  struct Options {
    std::string scenario = "all";
    std::vector<size_t> threads{1, 2, 4, 8, 12};
    std::optional<size_t> ops;  // total operations per run, all producers
    size_t repeat = 5;
    size_t capacity = 1u << 11;        // SinkToFile default
    size_t message_length = 1u << 10;  // SinkToFile default
    std::filesystem::path sink_path = "/dev/null";
  };

  struct RunResult {
    double seconds = 0;
    size_t ops = 0;
    std::vector<uint64_t> latencies;  // ns per operation
    bool ok = true;
    std::string error;
  };

  /// Start barrier: every participant checks in, then all are released at
  /// once by the main thread.
  class StartGate {
   public:
    explicit StartGate(size_t participants) : expected_(participants) {}

    void arriveAndWait() {
      ready_.fetch_add(1, std::memory_order_acq_rel);
      while (not go_.load(std::memory_order_acquire)) {
        cpuRelax();
      }
    }

    Clock::time_point open() {
      while (ready_.load(std::memory_order_acquire) != expected_) {
        std::this_thread::yield();
      }
      auto start = Clock::now();
      go_.store(true, std::memory_order_release);
      return start;
    }

   private:
    const size_t expected_;
    std::atomic_size_t ready_{0};
    std::atomic_bool go_{false};
  };

  inline uint64_t elapsedNs(Clock::time_point from, Clock::time_point to) {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(to - from)
            .count());
  }

  // ---------------------------------------------------------------- queue

  struct Payload {
    Payload(uint32_t producer, uint64_t seq) : producer(producer), seq(seq) {}
    uint32_t producer;
    uint64_t seq;
  };

  RunResult runQueue(size_t producers, size_t total_ops, const Options &opt) {
    const size_t per_producer = total_ops / producers;
    const size_t expected = per_producer * producers;

    soralog::CircularBuffer<Payload> buffer(opt.capacity, opt.message_length);
    StartGate gate(producers + 1);

    RunResult result;
    result.ops = expected;
    std::vector<std::vector<uint64_t>> latencies(producers);

    std::thread consumer([&] {
      std::vector<uint64_t> last(producers, 0);
      size_t consumed = 0;
      gate.arriveAndWait();
      while (consumed < expected) {
        if (auto node = buffer.get()) {
          const auto producer = node->producer;
          const auto seq = node->seq;
          if (producer >= producers or seq != last[producer] + 1) {
            if (result.ok) {
              std::ostringstream ss;
              ss << "order violation: producer " << producer << " seq " << seq
                 << " after " << (producer < producers ? last[producer] : 0);
              result.error = ss.str();
            }
            result.ok = false;
          }
          if (producer < producers) {
            last[producer] = seq;
          }
          ++consumed;
        } else {
          cpuRelax();
        }
      }
      if (buffer.size() != 0) {
        result.ok = false;
        result.error = "buffer is not empty after all elements consumed";
      }
    });

    std::vector<std::thread> threads;
    threads.reserve(producers);
    for (size_t p = 0; p < producers; ++p) {
      latencies[p].resize(per_producer);
      threads.emplace_back([&, p] {
        auto &lat = latencies[p];
        gate.arriveAndWait();
        for (uint64_t i = 1; i <= per_producer; ++i) {
          auto t0 = Clock::now();
          while (true) {
            auto node = buffer.put(Payload(static_cast<uint32_t>(p), i));
            if (node) {
              break;
            }
            cpuRelax();  // buffer full: consumer is draining it
          }
          lat[i - 1] = elapsedNs(t0, Clock::now());
        }
      });
    }

    auto start = gate.open();
    for (auto &t : threads) {
      t.join();
    }
    consumer.join();
    result.seconds =
        std::chrono::duration<double>(Clock::now() - start).count();

    for (auto &lat : latencies) {
      result.latencies.insert(result.latencies.end(), lat.begin(), lat.end());
    }
    return result;
  }

  // ----------------------------------------------------------------- sink

  size_t countLines(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    size_t lines = 0;
    std::vector<char> chunk(1 << 20);
    while (in) {
      in.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
      lines += std::count(chunk.data(), chunk.data() + in.gcount(), '\n');
    }
    return lines;
  }

  RunResult runSink(size_t producers,
                    size_t total_ops,
                    size_t latency_ms,
                    const Options &opt) {
    const size_t per_producer = total_ops / producers;
    const size_t expected = per_producer * producers;
    const bool verify = opt.sink_path != "/dev/null";

    if (verify) {
      std::filesystem::remove(opt.sink_path);
    }

    RunResult result;
    result.ops = expected;
    std::vector<std::vector<uint64_t>> latencies(producers);

    {
      auto sink = std::make_shared<soralog::SinkToFile>(
          "benchmark",
          soralog::Level::TRACE,
          opt.sink_path,
          soralog::Sink::ThreadInfoType::NONE,
          opt.capacity,
          opt.message_length,
          std::nullopt,  // buffer size: default
          latency_ms);

      StartGate gate(producers);
      std::vector<std::thread> threads;
      threads.reserve(producers);
      for (size_t p = 0; p < producers; ++p) {
        latencies[p].resize(per_producer);
        threads.emplace_back([&, p] {
          auto &lat = latencies[p];
          gate.arriveAndWait();
          for (size_t i = 1; i <= per_producer; ++i) {
            auto t0 = Clock::now();
            sink->push("benchmark",
                       soralog::Level::INFO,
                       "Benchmark message #{} from producer {}",
                       i,
                       p);
            lat[i - 1] = elapsedNs(t0, Clock::now());
          }
        });
      }

      auto start = gate.open();
      for (auto &t : threads) {
        t.join();
      }
      sink->flush();  // drain what is left, it is part of the work
      result.seconds =
          std::chrono::duration<double>(Clock::now() - start).count();
    }  // sink destroyed: worker joined, file closed

    if (verify) {
      auto lines = countLines(opt.sink_path);
      if (lines != expected) {
        result.ok = false;
        result.error = "expected " + std::to_string(expected) + " lines, got "
                     + std::to_string(lines);
      }
    }

    for (auto &lat : latencies) {
      result.latencies.insert(result.latencies.end(), lat.begin(), lat.end());
    }
    return result;
  }

  // -------------------------------------------------------------- report

  uint64_t percentile(std::vector<uint64_t> &v, double q) {
    if (v.empty()) {
      return 0;
    }
    auto idx = static_cast<size_t>(q * static_cast<double>(v.size() - 1));
    std::nth_element(
        v.begin(), v.begin() + static_cast<ptrdiff_t>(idx), v.end());
    return v[idx];
  }

  template <typename Run>
  bool measure(const std::string &scenario,
               size_t default_ops,
               const Options &opt,
               Run run) {
    const size_t total_ops = opt.ops.value_or(default_ops);
    bool all_ok = true;

    std::printf(
        "\n== %s: %zu ops per run, %zu repeats, capacity %zu, "
        "message length %zu\n",
        scenario.c_str(),
        total_ops,
        opt.repeat,
        opt.capacity,
        opt.message_length);
    std::printf("%8s %12s %21s %9s %9s %9s %9s %11s\n",
                "threads",
                "Mops/s med",
                "(min..max)",
                "p50 ns",
                "p99 ns",
                "p99.9 ns",
                "p99.99 ns",
                "max ns");

    for (auto producers : opt.threads) {
      std::vector<double> rates;
      std::vector<uint64_t> latencies;
      for (size_t r = 0; r < opt.repeat; ++r) {
        auto res = run(producers, total_ops);
        if (not res.ok) {
          all_ok = false;
          std::fprintf(stderr,
                       "FAILED %s threads=%zu repeat=%zu: %s\n",
                       scenario.c_str(),
                       producers,
                       r,
                       res.error.c_str());
        }
        rates.push_back(static_cast<double>(res.ops) / res.seconds / 1e6);
        latencies.insert(
            latencies.end(), res.latencies.begin(), res.latencies.end());
      }
      std::sort(rates.begin(), rates.end());
      auto median = rates[rates.size() / 2];

      char range[32];
      std::snprintf(
          range, sizeof(range), "(%.2f..%.2f)", rates.front(), rates.back());

      auto p50 = percentile(latencies, 0.50);
      auto p99 = percentile(latencies, 0.99);
      auto p999 = percentile(latencies, 0.999);
      auto p9999 = percentile(latencies, 0.9999);
      auto max = latencies.empty()
                   ? uint64_t{0}
                   : *std::max_element(latencies.begin(), latencies.end());

      std::printf("%8zu %12.2f %21s %9" PRIu64 " %9" PRIu64 " %9" PRIu64
                  " %9" PRIu64 " %11" PRIu64 "\n",
                  producers,
                  median,
                  range,
                  p50,
                  p99,
                  p999,
                  p9999,
                  max);
      std::fflush(stdout);
    }
    return all_ok;
  }

  std::vector<size_t> parseList(const std::string &s) {
    std::vector<size_t> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
      out.push_back(std::stoul(item));
    }
    return out;
  }

  [[noreturn]] void usage(const char *argv0) {
    std::fprintf(stderr,
                 "Usage: %s [--scenario queue|sink|sink-sync|all] "
                 "[--threads 1,2,4,8,12] [--ops N] [--repeat R] [--capacity N] "
                 "[--message-length N] [--sink-path PATH]\n",
                 argv0);
    std::exit(2);
  }

  Options parse(int argc, char **argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      auto value = [&]() -> std::string {
        if (i + 1 >= argc) {
          usage(argv[0]);
        }
        return argv[++i];
      };
      if (arg == "--scenario") {
        opt.scenario = value();
      } else if (arg == "--threads") {
        opt.threads = parseList(value());
      } else if (arg == "--ops") {
        opt.ops = std::stoul(value());
      } else if (arg == "--repeat") {
        opt.repeat = std::max<size_t>(1, std::stoul(value()));
      } else if (arg == "--capacity") {
        opt.capacity = std::stoul(value());
      } else if (arg == "--message-length") {
        opt.message_length = std::stoul(value());
      } else if (arg == "--sink-path") {
        opt.sink_path = value();
      } else {
        usage(argv[0]);
      }
    }
    if (opt.scenario != "all" and opt.scenario != "queue"
        and opt.scenario != "sink" and opt.scenario != "sink-sync") {
      usage(argv[0]);
    }
    if (opt.capacity == 0) {
      usage(argv[0]);
    }
    if (opt.threads.empty()
        or std::find(opt.threads.begin(), opt.threads.end(), 0)
               != opt.threads.end()) {
      usage(argv[0]);
    }
    return opt;
  }

}  // namespace

int main(int argc, char **argv) {
  auto opt = parse(argc, argv);

  std::printf(
      "soralog benchmark: hardware threads %u, "
#ifdef NDEBUG
      "release build"
#else
      "DEBUG BUILD - numbers are not representative"
#endif
      "\n",
      std::thread::hardware_concurrency());

  bool ok = true;
  auto want = [&](const char *name) {
    return opt.scenario == "all" or opt.scenario == name;
  };

  if (want("queue")) {
    ok &= measure("queue", 4'000'000, opt, [&](size_t producers, size_t ops) {
      return runQueue(producers, ops, opt);
    });
  }
  if (want("sink")) {
    ok &= measure("sink (latency 1000 ms)",
                  1'000'000,
                  opt,
                  [&](size_t producers, size_t ops) {
                    return runSink(producers, ops, 1000, opt);
                  });
  }
  if (want("sink-sync")) {
    ok &= measure("sink-sync (latency 0)",
                  250'000,
                  opt,
                  [&](size_t producers, size_t ops) {
                    return runSink(producers, ops, 0, opt);
                  });
  }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

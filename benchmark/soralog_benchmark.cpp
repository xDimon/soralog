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
 *  - load:      N producers -> SinkToFile with every setting given, driven by
 *               the load generator of load_driver.hpp: closed loop or open
 *               loop at a given rate, in bursts, with a given message size;
 *               reports push time, response time from the scheduled moment,
 *               delivery time to the file and CPU of the sink's worker.
 *               Not part of "all".
 *
 * For each scenario and producer count the run is repeated; throughput is
 * reported as median (min..max) over repeats, latency percentiles of a single
 * operation are computed over all operations of all repeats. Latency includes
 * the cost of reading the clock twice per operation; for the load scenario
 * that overhead is measured on an empty target and printed first.
 *
 * Usage:
 *   soralog_benchmark [--scenario queue|sink|sink-sync|load|all]
 *                     [--threads 1,2,4,8,12] [--ops N] [--repeat R]
 *                     [--capacity N] [--message-length N]
 *                     [--buffer-size N] [--sink-path PATH]
 *   load only:        [--latency MS] [--rate EVENTS_PER_SEC] [--burst N]
 *                     [--message-size N] [--warmup N] [--duration SEC]
 *                     [--delivery] [--poll-us N]
 *
 * With --sink-path other than /dev/null the output file is recreated for each
 * run and verified: the line count for sink scenarios; for the load scenario
 * every event of every producer, once and in order. --delivery follows the
 * file during the run and needs a real file.
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

#include <soralog/circular_buffer.hpp>
#include <soralog/impl/sink_to_file.hpp>

#include "load_driver.hpp"

namespace {

  using bench::Clock;
  using bench::cpuRelax;
  using bench::elapsedNs;
  using bench::StartGate;

  struct Options {
    std::string scenario = "all";
    std::vector<size_t> threads{1, 2, 4, 8, 12};
    std::optional<size_t> ops;  // total operations per run, all producers
    size_t repeat = 5;
    size_t capacity = 1u << 11;         // SinkToFile default
    size_t message_length = 1u << 10;   // SinkToFile default
    std::optional<size_t> buffer_size;  // SinkToFile default if not set
    std::filesystem::path sink_path = "/dev/null";
    // load scenario
    size_t latency_ms = 1000;  // SinkToFile default
    double rate = 0;           // events/s, all producers; 0 - max
    size_t burst = 1;
    size_t message_size = 64;
    std::optional<size_t> warmup;  // total; capacity if not set
    double duration = 5;           // seconds of open loop without --ops
    bool delivery = false;
    size_t poll_us = 100;
  };

  struct RunResult {
    double seconds = 0;
    size_t ops = 0;
    std::vector<uint64_t> latencies;  // ns per operation
    bool ok = true;
    std::string error;
  };

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
          opt.buffer_size,
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

  // ----------------------------------------------------------------- load

  struct SoralogTarget {
    explicit SoralogTarget(const Options &opt)
        : sink(std::make_shared<soralog::SinkToFile>(
              "benchmark",
              soralog::Level::TRACE,
              opt.sink_path,
              soralog::Sink::ThreadInfoType::NONE,
              opt.capacity,
              opt.message_length,
              opt.buffer_size,
              opt.latency_ms)) {}

    void push(std::string_view message) {
      sink->push("benchmark", soralog::Level::INFO, "{}", message);
    }

    void flush() {
      sink->flush();
    }

    std::shared_ptr<soralog::SinkToFile> sink;
  };

  bench::LoadSpec loadSpec(const Options &opt, size_t producers) {
    bench::LoadSpec spec;
    spec.producers = producers;
    spec.events = opt.ops.value_or(
        opt.rate > 0 ? static_cast<size_t>(opt.rate * opt.duration)
                     : 1'000'000);
    spec.events = std::max(spec.events, producers);
    const size_t warmup = opt.warmup.value_or(opt.capacity);
    spec.warmup = (warmup + producers - 1) / producers;
    spec.rate = opt.rate;
    spec.burst = opt.burst;
    spec.message_size = opt.message_size;
    spec.path = opt.sink_path;
    spec.track_delivery = opt.delivery;
    spec.poll_interval = std::chrono::microseconds(opt.poll_us);
    return spec;
  }

  /// Formats nanoseconds as microseconds with precision fitting the value.
  std::string us(uint64_t ns) {
    const double value = static_cast<double>(ns) / 1e3;
    char text[32];
    std::snprintf(text,
                  sizeof(text),
                  value < 10     ? "%.2f"
                  : value < 1000 ? "%.1f"
                                 : "%.0f",
                  value);
    return text;
  }

  bool measureLoad(const Options &opt) {
    char rate[64];
    if (opt.rate > 0) {
      std::snprintf(rate, sizeof(rate), "open loop %.0f ev/s", opt.rate);
    } else {
      std::snprintf(rate, sizeof(rate), "closed loop");
    }
    std::printf(
        "\n== load: %s, burst %zu, message size %zu, %zu repeats\n"
        "   sink: capacity %zu, message length %zu, buffer size %s, "
        "latency %zu ms; warmup %zu events, delivery %s\n",
        rate,
        opt.burst,
        std::max(opt.message_size, bench::Message::kHeaderSize),
        opt.repeat,
        opt.capacity,
        opt.message_length,
        opt.buffer_size ? std::to_string(*opt.buffer_size).c_str() : "default",
        opt.latency_ms,
        opt.warmup.value_or(opt.capacity),
        opt.delivery ? "tracked" : "not tracked");
    if (opt.message_size > opt.message_length) {
      std::printf("   messages are truncated to %zu bytes by the sink\n",
                  opt.message_length);
    }

    // Overhead of the driver itself: clock reads, message stamping
    {
      auto spec = loadSpec(opt, 1);
      spec.path.clear();
      spec.track_delivery = false;
      spec.rate = 0;
      spec.events = 1'000'000;
      spec.warmup = 0;
      auto res = bench::runLoad(
          spec, [] { return std::make_unique<bench::NullTarget>(); });
      std::printf(
          "   driver overhead per push (empty target): p50 %s us, "
          "p99 %s us\n",
          us(res.service.percentile(0.5)).c_str(),
          us(res.service.percentile(0.99)).c_str());
    }

    std::printf(
        "   times in us; resp: from the scheduled moment; dlv: "
        "until readable from the file; bg cpu: sink worker, %% of "
        "a core\n");
    std::printf("%7s %10s %9s %9s %9s %9s %10s %10s %10s %9s %9s %9s %7s %7s\n",
                "threads",
                "kev/s med",
                "push p50",
                "push p99",
                "p99.9",
                "max",
                "resp p99",
                "p99.9",
                "max",
                "dlv p50",
                "p99",
                "max",
                "bg cpu",
                "B/line");

    bool all_ok = true;
    for (auto producers : opt.threads) {
      const auto spec = loadSpec(opt, producers);
      bench::Histogram service;
      bench::Histogram response;
      bench::Histogram delivery;
      std::vector<double> rates;
      std::vector<double> cpus;
      double bytes_per_line = 0;
      for (size_t r = 0; r < opt.repeat; ++r) {
        auto res = bench::runLoad(
            spec, [&] { return std::make_unique<SoralogTarget>(opt); });
        if (not res.ok) {
          all_ok = false;
          std::fprintf(stderr,
                       "FAILED load threads=%zu repeat=%zu: %s\n",
                       producers,
                       r,
                       res.error.c_str());
        }
        rates.push_back(static_cast<double>(res.events) / res.seconds / 1e3);
        cpus.push_back(res.background_cpu * 100);
        service.merge(res.service);
        response.merge(res.response);
        delivery.merge(res.delivery);
        if (res.verified and res.lines != 0) {
          bytes_per_line =
              static_cast<double>(res.bytes) / static_cast<double>(res.lines);
        }
      }
      std::sort(rates.begin(), rates.end());
      std::sort(cpus.begin(), cpus.end());

      auto dlv = [&](double q) {
        return delivery.count() == 0 ? std::string("-")
             : q < 1                 ? us(delivery.percentile(q))
                                     : us(delivery.max());
      };
      std::printf(
          "%7zu %10.1f %9s %9s %9s %9s %10s %10s %10s %9s %9s %9s %7.1f "
          "%7.0f\n",
          producers,
          rates[rates.size() / 2],
          us(service.percentile(0.5)).c_str(),
          us(service.percentile(0.99)).c_str(),
          us(service.percentile(0.999)).c_str(),
          us(service.max()).c_str(),
          us(response.percentile(0.99)).c_str(),
          us(response.percentile(0.999)).c_str(),
          us(response.max()).c_str(),
          dlv(0.5).c_str(),
          dlv(0.99).c_str(),
          dlv(1).c_str(),
          cpus[cpus.size() / 2],
          bytes_per_line);
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
                 "Usage: %s [--scenario queue|sink|sink-sync|load|all] "
                 "[--threads 1,2,4,8,12] [--ops N] [--repeat R] [--capacity N] "
                 "[--message-length N] [--buffer-size N] [--sink-path PATH]\n"
                 "  load only: [--latency MS] [--rate EVENTS_PER_SEC] "
                 "[--burst N] [--message-size N] [--warmup N] "
                 "[--duration SEC] [--delivery] [--poll-us N]\n",
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
      } else if (arg == "--buffer-size") {
        opt.buffer_size = std::stoul(value());
      } else if (arg == "--sink-path") {
        opt.sink_path = value();
      } else if (arg == "--latency") {
        opt.latency_ms = std::stoul(value());
      } else if (arg == "--rate") {
        opt.rate = std::stod(value());
      } else if (arg == "--burst") {
        opt.burst = std::stoul(value());
      } else if (arg == "--message-size") {
        opt.message_size = std::stoul(value());
      } else if (arg == "--warmup") {
        opt.warmup = std::stoul(value());
      } else if (arg == "--duration") {
        opt.duration = std::stod(value());
      } else if (arg == "--delivery") {
        opt.delivery = true;
      } else if (arg == "--poll-us") {
        opt.poll_us = std::stoul(value());
      } else {
        usage(argv[0]);
      }
    }
    if (opt.scenario != "all" and opt.scenario != "queue"
        and opt.scenario != "sink" and opt.scenario != "sink-sync"
        and opt.scenario != "load") {
      usage(argv[0]);
    }
    if (opt.capacity == 0 or opt.burst == 0 or opt.rate < 0
        or opt.duration <= 0) {
      usage(argv[0]);
    }
    if (opt.delivery and opt.sink_path == "/dev/null") {
      std::fprintf(stderr, "--delivery needs --sink-path to a real file\n");
      std::exit(2);
    }
    if (opt.scenario == "load" and opt.sink_path != "/dev/null"
        and opt.message_length < bench::Message::kHeaderSize) {
      std::fprintf(stderr,
                   "--message-length below %zu cuts the event header, the "
                   "output can't be verified\n",
                   bench::Message::kHeaderSize);
      std::exit(2);
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
  if (opt.scenario == "load") {
    ok &= measureLoad(opt);
  }

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

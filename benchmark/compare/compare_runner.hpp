/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file compare_runner.hpp
 * @brief Common command line, run loop and output of the comparison
 * benchmarks: one executable per logger, same options, same table, same CSV.
 *
 * Usage of every compare_* executable:
 *   [--threads 1,2,4] [--ops N] [--repeat R] [--rate EVENTS_PER_SEC]
 *   [--burst N] [--message-size N] [--warmup N] [--duration SEC]
 *   [--delivery] [--poll-us N] [--sink-path PATH]
 *   [--capacity N] [--message-length N] [--buffer-size N] [--latency MS]
 *   [--mode NAME] [--csv PATH] [--label NAME]
 *
 * Logger settings are mapped by each adapter as close as the logger allows;
 * what is mapped and how is printed in the header of the run and documented
 * in docs/benchmark.
 */

#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../load_driver.hpp"

namespace bench::compare {

  struct Options {
    std::vector<size_t> threads{1, 2, 4, 8};
    std::optional<size_t> ops;  // measured events per run, all producers
    size_t repeat = 3;
    double rate = 0;  // events/s, all producers; 0 - closed loop
    size_t burst = 1;
    size_t message_size = 64;
    std::optional<size_t> warmup;  // total; capacity if not set
    double duration = 3;           // seconds of open loop without --ops
    bool delivery = false;
    size_t poll_us = 100;
    std::filesystem::path sink_path = "/dev/null";
    // Logger settings, mapped by the adapter
    size_t capacity = 1u << 11;         // events in the queue
    size_t message_length = 1u << 10;   // message length limit
    std::optional<size_t> buffer_size;  // write buffer, bytes
    size_t latency_ms = 1000;           // longest delay before writing out
    std::string mode;                   // adapter-specific variant
    std::filesystem::path csv;
    std::string label;
  };

  [[noreturn]] inline void usage(const char *argv0) {
    std::fprintf(
        stderr,
        "Usage: %s [--threads 1,2,4] [--ops N] [--repeat R] "
        "[--rate EVENTS_PER_SEC] [--burst N] [--message-size N] [--warmup N] "
        "[--duration SEC] [--delivery] [--poll-us N] [--sink-path PATH] "
        "[--capacity N] [--message-length N] [--buffer-size N] [--latency MS] "
        "[--mode NAME] [--csv PATH] [--label NAME]\n",
        argv0);
    std::exit(2);
  }

  inline std::vector<size_t> parseList(const std::string &s) {
    std::vector<size_t> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
      out.push_back(std::stoul(item));
    }
    return out;
  }

  inline Options parse(int argc, char **argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      auto value = [&]() -> std::string {
        if (i + 1 >= argc) {
          usage(argv[0]);
        }
        return argv[++i];
      };
      if (arg == "--threads") {
        opt.threads = parseList(value());
      } else if (arg == "--ops") {
        opt.ops = std::stoul(value());
      } else if (arg == "--repeat") {
        opt.repeat = std::max<size_t>(1, std::stoul(value()));
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
      } else if (arg == "--sink-path") {
        opt.sink_path = value();
      } else if (arg == "--capacity") {
        opt.capacity = std::stoul(value());
      } else if (arg == "--message-length") {
        opt.message_length = std::stoul(value());
      } else if (arg == "--buffer-size") {
        opt.buffer_size = std::stoul(value());
      } else if (arg == "--latency") {
        opt.latency_ms = std::stoul(value());
      } else if (arg == "--mode") {
        opt.mode = value();
      } else if (arg == "--csv") {
        opt.csv = value();
      } else if (arg == "--label") {
        opt.label = value();
      } else {
        usage(argv[0]);
      }
    }
    if (opt.capacity == 0 or opt.burst == 0 or opt.rate < 0 or opt.duration <= 0
        or opt.threads.empty()
        or std::find(opt.threads.begin(), opt.threads.end(), 0)
               != opt.threads.end()) {
      usage(argv[0]);
    }
    if (opt.delivery and opt.sink_path == "/dev/null") {
      std::fprintf(stderr, "--delivery needs --sink-path to a real file\n");
      std::exit(2);
    }
    if (opt.sink_path != "/dev/null"
        and opt.message_length < Message::kHeaderSize) {
      std::fprintf(stderr,
                   "--message-length below %zu cuts the event header, the "
                   "output can't be verified\n",
                   Message::kHeaderSize);
      std::exit(2);
    }
    return opt;
  }

  inline LoadSpec loadSpec(const Options &opt, size_t producers) {
    LoadSpec spec;
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
  inline std::string us(uint64_t ns) {
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

  /// Results of all repeats of one thread count.
  struct Summary {
    size_t threads = 0;
    size_t events = 0;
    Histogram service;
    Histogram response;
    Histogram delivery;
    std::vector<double> rates;  // events/s
    std::vector<double> cpus;   // % of a core
    double bytes_per_line = 0;
    bool ok = true;

    double median(std::vector<double> v) const {
      std::sort(v.begin(), v.end());
      return v.empty() ? 0 : v[v.size() / 2];
    }
  };

  inline void writeCsv(const Options &opt,
                       const std::string &logger,
                       const std::string &version,
                       const Histogram &overhead,
                       const Summary &s) {
    if (opt.csv.empty()) {
      return;
    }
    const bool header = not std::filesystem::exists(opt.csv)
                     or std::filesystem::file_size(opt.csv) == 0;
    std::ofstream out(opt.csv, std::ios::app);
    out << std::fixed << std::setprecision(1);
    if (header) {
      out << "label,logger,version,threads,rate,burst,message_size,capacity,"
             "message_length,buffer_size,latency_ms,events,repeats,"
             "rate_med,rate_min,rate_max,push_p50_ns,push_p99_ns,"
             "push_p999_ns,push_max_ns,resp_p99_ns,resp_p999_ns,resp_max_ns,"
             "dlv_p50_ns,dlv_p99_ns,dlv_max_ns,bg_cpu_pct,bytes_per_line,"
             "overhead_p50_ns,overhead_p99_ns,ok\n";
    }
    auto rates = s.rates;
    std::sort(rates.begin(), rates.end());
    const bool dlv = s.delivery.count() != 0;
    auto opt_ns = [&](uint64_t v) {
      return dlv ? std::to_string(v) : std::string();
    };
    out << opt.label << ',' << logger << ',' << version << ',' << s.threads
        << ',' << opt.rate << ',' << opt.burst << ','
        << std::max(opt.message_size, Message::kHeaderSize) << ','
        << opt.capacity << ',' << opt.message_length << ','
        << (opt.buffer_size ? std::to_string(*opt.buffer_size) : "") << ','
        << opt.latency_ms << ',' << s.events << ',' << opt.repeat << ','
        << s.median(s.rates) << ',' << rates.front() << ',' << rates.back()
        << ',' << s.service.percentile(0.5) << ',' << s.service.percentile(0.99)
        << ',' << s.service.percentile(0.999) << ',' << s.service.max() << ','
        << s.response.percentile(0.99) << ',' << s.response.percentile(0.999)
        << ',' << s.response.max() << ',' << opt_ns(s.delivery.percentile(0.5))
        << ',' << opt_ns(s.delivery.percentile(0.99)) << ','
        << opt_ns(s.delivery.max()) << ',' << s.median(s.cpus) << ','
        << s.bytes_per_line << ',' << overhead.percentile(0.5) << ','
        << overhead.percentile(0.99) << ',' << (s.ok ? 1 : 0) << '\n';
  }

  /**
   * @brief Runs the load for every thread count, prints the table, appends
   * CSV rows.
   * @param settings Line describing how the logger settings are mapped.
   * @param make Creates the logging target, see runLoad().
   * @return Process exit code.
   */
  template <typename Make>
  int run(const Options &opt,
          const std::string &logger,
          const std::string &version,
          const std::string &settings,
          Make make) {
    char rate[64];
    if (opt.rate > 0) {
      std::snprintf(rate, sizeof(rate), "open loop %.0f ev/s", opt.rate);
    } else {
      std::snprintf(rate, sizeof(rate), "closed loop");
    }
    std::printf(
        "== %s %s%s%s: %s, burst %zu, message size %zu, %zu repeats, "
        "%s, hardware threads %u\n   %s\n",
        logger.c_str(),
        version.c_str(),
        opt.label.empty() ? "" : ", ",
        opt.label.c_str(),
        rate,
        opt.burst,
        std::max(opt.message_size, Message::kHeaderSize),
        opt.repeat,
#ifdef NDEBUG
        "release build",
#else
        "DEBUG BUILD - numbers are not representative",
#endif
        std::thread::hardware_concurrency(),
        settings.c_str());

    // Overhead of the driver itself: clock reads, message stamping
    Histogram overhead;
    {
      auto spec = loadSpec(opt, 1);
      spec.path.clear();
      spec.track_delivery = false;
      spec.rate = 0;
      spec.events = 1'000'000;
      spec.warmup = 0;
      overhead =
          runLoad(spec, [] { return std::make_unique<NullTarget>(); }).service;
    }
    std::printf(
        "   driver overhead per push: p50 %s us, p99 %s us; times in "
        "us, resp from the scheduled moment, dlv until readable from "
        "the file, bg cpu of the logger's threads in %% of a core\n",
        us(overhead.percentile(0.5)).c_str(),
        us(overhead.percentile(0.99)).c_str());
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
      Summary s;
      s.threads = producers;
      for (size_t r = 0; r < opt.repeat; ++r) {
        auto res = runLoad(spec, make);
        if (not res.ok) {
          s.ok = false;
          std::fprintf(stderr,
                       "FAILED %s threads=%zu repeat=%zu: %s\n",
                       logger.c_str(),
                       producers,
                       r,
                       res.error.c_str());
        }
        s.events = res.events;
        s.rates.push_back(static_cast<double>(res.events) / res.seconds);
        s.cpus.push_back(res.background_cpu * 100);
        s.service.merge(res.service);
        s.response.merge(res.response);
        s.delivery.merge(res.delivery);
        if (res.verified and res.lines != 0) {
          s.bytes_per_line =
              static_cast<double>(res.bytes) / static_cast<double>(res.lines);
        }
      }
      all_ok &= s.ok;

      auto dlv = [&](double q) {
        return s.delivery.count() == 0 ? std::string("-")
             : q < 1                   ? us(s.delivery.percentile(q))
                                       : us(s.delivery.max());
      };
      std::printf(
          "%7zu %10.1f %9s %9s %9s %9s %10s %10s %10s %9s %9s %9s %7.1f "
          "%7.0f\n",
          producers,
          s.median(s.rates) / 1e3,
          us(s.service.percentile(0.5)).c_str(),
          us(s.service.percentile(0.99)).c_str(),
          us(s.service.percentile(0.999)).c_str(),
          us(s.service.max()).c_str(),
          us(s.response.percentile(0.99)).c_str(),
          us(s.response.percentile(0.999)).c_str(),
          us(s.response.max()).c_str(),
          dlv(0.5).c_str(),
          dlv(0.99).c_str(),
          dlv(1).c_str(),
          s.median(s.cpus),
          s.bytes_per_line);
      std::fflush(stdout);
      writeCsv(opt, logger, version, overhead, s);
    }
    return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
  }

}  // namespace bench::compare

/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file load_driver.hpp
 * @brief Logger-agnostic load generator for benchmarks.
 *
 * Drives a logging target with N producer threads and measures what the
 * application pays and when the events reach the file:
 *  - closed loop (rate 0): every producer pushes as fast as it can; gives
 *    the saturation throughput;
 *  - open loop (rate > 0): events are scheduled at the given total rate,
 *    optionally in bursts; response time is counted from the scheduled
 *    moment, so the time a producer spends stuck in the logger is charged to
 *    the events delayed behind it (no coordinated omission).
 *
 * Every message starts with a fixed-width header: producer, sequence number
 * and the moment of the push. It lets the driver verify the output file
 * (each producer's events present, once, in order) and measure the delivery
 * time: a reader thread follows the growing file and compares the embedded
 * moment with the moment the line became readable. The resolution of the
 * delivery time is the poll interval of the reader.
 *
 * A target is any type with `void push(std::string_view)` and `void flush()`,
 * where flush() returns when everything pushed before is readable from the
 * file. Destroying the target finishes the output.
 */

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#if defined(__x86_64__) or defined(_M_X64)
#include <immintrin.h>
#endif

namespace bench {

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

  inline uint64_t elapsedNs(Clock::time_point from, Clock::time_point to) {
    if (to <= from) {
      return 0;
    }
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(to - from)
            .count());
  }

  inline uint64_t cpuNs(clockid_t clock) {
    timespec ts{};
    clock_gettime(clock, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000u
         + static_cast<uint64_t>(ts.tv_nsec);
  }

  /// CPU time consumed by the calling thread.
  inline uint64_t threadCpuNs() {
    return cpuNs(CLOCK_THREAD_CPUTIME_ID);
  }

  /// CPU time consumed by the whole process.
  inline uint64_t processCpuNs() {
    return cpuNs(CLOCK_PROCESS_CPUTIME_ID);
  }

  /// Waits until the moment: sleeps through most of a long wait, spins the
  /// rest, since sleeping is too coarse for sub-millisecond schedules.
  inline void waitUntil(Clock::time_point moment) {
    constexpr auto kSpin = std::chrono::microseconds(200);
    const auto now = Clock::now();
    if (moment - now > 2 * kSpin) {
      std::this_thread::sleep_for(moment - now - kSpin);
    }
    while (Clock::now() < moment) {
      cpuRelax();
    }
  }

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

    /// Waits until every participant has checked in.
    void waitReady() const {
      while (ready_.load(std::memory_order_acquire) != expected_) {
        std::this_thread::yield();
      }
    }

    Clock::time_point open() {
      waitReady();
      start_ = Clock::now();
      go_.store(true, std::memory_order_release);
      return start_;
    }

    /// Moment of opening; valid for participants after arriveAndWait().
    Clock::time_point start() const {
      return start_;
    }

   private:
    const size_t expected_;
    std::atomic_size_t ready_{0};
    std::atomic_bool go_{false};
    Clock::time_point start_{};
  };

  /**
   * @brief Log-linear histogram of nanosecond values.
   *
   * Values below 16 are exact; above, every power of two is split into 16
   * buckets, so a reported percentile is at most ~6% above the true value.
   */
  class Histogram {
   public:
    void record(uint64_t value) noexcept {
      ++counts_[index(value)];
      ++total_;
      max_ = std::max(max_, value);
    }

    void merge(const Histogram &other) noexcept {
      for (size_t i = 0; i < counts_.size(); ++i) {
        counts_[i] += other.counts_[i];
      }
      total_ += other.total_;
      max_ = std::max(max_, other.max_);
    }

    uint64_t count() const noexcept {
      return total_;
    }

    uint64_t max() const noexcept {
      return max_;
    }

    /// Upper bound of the bucket holding the q-th quantile, q in [0, 1].
    uint64_t percentile(double q) const noexcept {
      if (total_ == 0) {
        return 0;
      }
      auto rank = static_cast<uint64_t>(q * static_cast<double>(total_));
      rank = std::clamp<uint64_t>(rank, 1, total_);
      uint64_t seen = 0;
      for (size_t i = 0; i < counts_.size(); ++i) {
        seen += counts_[i];
        if (seen >= rank) {
          return std::min(upperBound(i), max_);
        }
      }
      return max_;
    }

   private:
    static constexpr unsigned kSubBits = 4;
    static constexpr uint64_t kSub = uint64_t{1} << kSubBits;

    static size_t index(uint64_t value) noexcept {
      if (value < kSub) {
        return static_cast<size_t>(value);
      }
      const auto msb = 63u - static_cast<unsigned>(__builtin_clzll(value));
      const auto shift = msb - kSubBits;
      const auto sub = (value >> shift) & (kSub - 1);
      return static_cast<size_t>((shift + 1) * kSub + sub);
    }

    static uint64_t upperBound(size_t index) noexcept {
      if (index < kSub) {
        return index;
      }
      const auto shift = index / kSub - 1;
      const auto sub = index % kSub;
      return ((kSub + sub + 1) << shift) - 1;
    }

    std::array<uint64_t, 64 * kSub> counts_{};
    uint64_t total_ = 0;
    uint64_t max_ = 0;
  };

  /**
   * @brief Message text of one producer: header, then padding.
   *
   * Header: '@', producer (4 hex), ':', sequence number (12 hex), ':',
   * moment of the push (16 hex, ns of Clock), ' '.
   */
  class Message {
   public:
    static constexpr size_t kHeaderSize = 36;

    Message(size_t producer, size_t size)
        : text_(std::max(size, kHeaderSize), '.') {
      text_[0] = '@';
      putHex(&text_[1], producer, 4);
      text_[5] = ':';
      text_[18] = ':';
      text_[35] = ' ';
    }

    std::string_view stamp(uint64_t seq, Clock::time_point moment) noexcept {
      putHex(&text_[6], seq, 12);
      putHex(&text_[19],
             static_cast<uint64_t>(
                 std::chrono::duration_cast<std::chrono::nanoseconds>(
                     moment.time_since_epoch())
                     .count()),
             16);
      return text_;
    }

    struct Parsed {
      uint64_t producer;
      uint64_t seq;
      Clock::time_point moment;
    };

    /// Finds and parses the header in a line of the output file.
    static bool parse(std::string_view line, Parsed &out) noexcept {
      const auto at = line.find('@');
      if (at == std::string_view::npos or line.size() - at < kHeaderSize) {
        return false;
      }
      const char *p = line.data() + at;
      uint64_t ns = 0;
      if (p[5] != ':' or p[18] != ':' or not getHex(p + 1, 4, out.producer)
          or not getHex(p + 6, 12, out.seq) or not getHex(p + 19, 16, ns)) {
        return false;
      }
      out.moment = Clock::time_point(std::chrono::nanoseconds(ns));
      return true;
    }

   private:
    static void putHex(char *p, uint64_t value, size_t digits) noexcept {
      static constexpr char kDigits[] = "0123456789abcdef";
      for (size_t i = digits; i-- > 0;) {
        p[i] = kDigits[value & 15];
        value >>= 4;
      }
    }

    static bool getHex(const char *p, size_t digits, uint64_t &out) noexcept {
      out = 0;
      for (size_t i = 0; i < digits; ++i) {
        const char c = p[i];
        uint64_t d = 0;
        if (c >= '0' and c <= '9') {
          d = static_cast<uint64_t>(c - '0');
        } else if (c >= 'a' and c <= 'f') {
          d = static_cast<uint64_t>(c - 'a' + 10);
        } else {
          return false;
        }
        out = (out << 4) | d;
      }
      return true;
    }

    std::string text_;
  };

  /**
   * @brief Checks the output file line by line: every producer's events
   * present, once, in order; optionally records their delivery time.
   */
  class OutputChecker {
   public:
    OutputChecker(size_t producers, size_t warmup_per_producer)
        : last_(producers, 0), warmup_(warmup_per_producer) {}

    /// Feeds a chunk of the file read at the moment `now`.
    void feed(const char *data,
              size_t size,
              Clock::time_point now,
              Histogram *delivery) {
      bytes_ += size;
      const char *const end = data + size;
      while (data < end) {
        const auto *nl = static_cast<const char *>(
            std::memchr(data, '\n', static_cast<size_t>(end - data)));
        if (nl == nullptr) {
          partial_.append(data, end);
          break;
        }
        if (partial_.empty()) {
          line({data, static_cast<size_t>(nl - data)}, now, delivery);
        } else {
          partial_.append(data, nl);
          line(partial_, now, delivery);
          partial_.clear();
        }
        data = nl + 1;
      }
    }

    /// Feeds the whole file at once, without delivery time.
    bool feedFile(const std::filesystem::path &path) {
      const int fd = ::open(path.c_str(), O_RDONLY);
      if (fd < 0) {
        fail("can't open " + path.string());
        return false;
      }
      std::vector<char> buffer(1 << 20);
      ssize_t n = 0;
      while ((n = ::read(fd, buffer.data(), buffer.size())) > 0) {
        feed(buffer.data(), static_cast<size_t>(n), Clock::now(), nullptr);
      }
      ::close(fd);
      return true;
    }

    /// Checks that every producer has delivered `per_producer` events.
    void finish(size_t per_producer) {
      if (not partial_.empty()) {
        fail("unterminated last line");
      }
      for (size_t p = 0; p < last_.size(); ++p) {
        if (last_[p] != per_producer) {
          fail("producer " + std::to_string(p) + ": " + std::to_string(last_[p])
               + " events of " + std::to_string(per_producer));
        }
      }
    }

    bool ok() const {
      return error_.empty();
    }
    const std::string &error() const {
      return error_;
    }
    uint64_t lines() const {
      return lines_;
    }
    uint64_t foreignLines() const {
      return foreign_;
    }
    uint64_t bytes() const {
      return bytes_;
    }

   private:
    void line(std::string_view text,
              Clock::time_point now,
              Histogram *delivery) {
      ++lines_;
      Message::Parsed parsed{};
      if (not Message::parse(text, parsed)) {
        ++foreign_;  // A line of the logger itself, or a truncated message
        return;
      }
      if (parsed.producer >= last_.size()) {
        fail("unknown producer " + std::to_string(parsed.producer));
        return;
      }
      auto &last = last_[parsed.producer];
      if (parsed.seq != last + 1) {
        fail("producer " + std::to_string(parsed.producer) + ": event "
             + std::to_string(parsed.seq) + " after " + std::to_string(last));
      }
      last = parsed.seq;
      if (delivery != nullptr and parsed.seq > warmup_) {
        delivery->record(elapsedNs(parsed.moment, now));
      }
    }

    void fail(std::string error) {
      if (error_.empty()) {
        error_ = std::move(error);
      }
    }

    std::vector<uint64_t> last_;
    const size_t warmup_;
    std::string partial_;
    std::string error_;
    uint64_t lines_ = 0;
    uint64_t foreign_ = 0;
    uint64_t bytes_ = 0;
  };

  struct LoadSpec {
    size_t producers = 1;
    size_t events = 0;        ///< Measured events, all producers together
    size_t warmup = 0;        ///< Unmeasured events before, per producer
    double rate = 0;          ///< Events per second, all producers; 0 - max
    size_t burst = 1;         ///< Events pushed back to back per schedule tick
    size_t message_size = 0;  ///< Message length, at least the header
    /// Output file to verify; empty or /dev/null - no verification
    std::filesystem::path path;
    bool track_delivery = false;  ///< Follow the file during the run
    std::chrono::microseconds poll_interval{100};
  };

  struct LoadResult {
    double seconds = 0;  ///< From the start to the end of the final flush
    size_t events = 0;   ///< Measured events
    Histogram service;   ///< Duration of push(), ns
    Histogram response;  ///< From the scheduled moment to the end of push()
    Histogram delivery;  ///< From the push to being readable from the file
    double background_cpu = 0;  ///< CPU of the logger's own threads, cores
    uint64_t lines = 0;
    uint64_t foreign_lines = 0;
    uint64_t bytes = 0;
    bool verified = false;
    bool ok = true;
    std::string error;
  };

  /**
   * @brief Runs one load test.
   * @param spec Load parameters.
   * @param make Creates the target, returns std::unique_ptr to it; the target
   * is destroyed before the file is verified.
   */
  template <typename Make>
  LoadResult runLoad(const LoadSpec &spec, Make make) {
    const size_t producers = spec.producers;
    const size_t per_producer = spec.events / producers;
    const bool verify = not spec.path.empty() and spec.path != "/dev/null";
    const bool open_loop = spec.rate > 0;
    const size_t burst = std::max<size_t>(1, spec.burst);
    // Interval between the bursts of one producer; producers are staggered
    const double interval_ns =
        open_loop ? 1e9 * static_cast<double>(producers * burst) / spec.rate
                  : 0;

    if (verify) {
      std::filesystem::remove(spec.path);
    }

    LoadResult result;
    result.events = per_producer * producers;

    auto target = make();
    OutputChecker checker(producers, spec.warmup);

    // Reader following the file; measures delivery and checks the output
    std::atomic_bool measuring{false};
    std::atomic_bool stop{false};
    uint64_t reader_cpu = 0;
    std::thread reader;
    if (verify and spec.track_delivery) {
      reader = std::thread([&] {
        std::vector<char> buffer(1 << 20);
        int fd = -1;
        bool was_measuring = false;
        uint64_t cpu_start = 0;
        while (true) {
          const bool stopping = stop.load(std::memory_order_acquire);
          if (fd < 0) {
            fd = ::open(spec.path.c_str(), O_RDONLY);
          }
          if (fd >= 0) {
            ssize_t n = 0;
            while ((n = ::read(fd, buffer.data(), buffer.size())) > 0) {
              checker.feed(buffer.data(),
                           static_cast<size_t>(n),
                           Clock::now(),
                           &result.delivery);
            }
          }
          const bool now_measuring = measuring.load(std::memory_order_acquire);
          if (now_measuring and not was_measuring) {
            cpu_start = threadCpuNs();
          } else if (was_measuring and not now_measuring) {
            reader_cpu = threadCpuNs() - cpu_start;
          }
          was_measuring = now_measuring;
          if (stopping) {
            break;
          }
          std::this_thread::sleep_for(spec.poll_interval);
        }
        if (fd >= 0) {
          ::close(fd);
        }
      });
    }

    StartGate gate(producers);
    std::vector<Histogram> service(producers);
    std::vector<Histogram> response(producers);
    std::vector<uint64_t> producer_cpu(producers, 0);
    std::vector<std::thread> threads;
    threads.reserve(producers);
    for (size_t p = 0; p < producers; ++p) {
      threads.emplace_back([&, p] {
        auto &out = *target;
        Message message(p, spec.message_size);
        uint64_t seq = 0;
        for (size_t i = 0; i < spec.warmup; ++i) {
          out.push(message.stamp(++seq, Clock::now()));
        }
        gate.arriveAndWait();
        const auto start = gate.start();
        const auto cpu_start = threadCpuNs();
        const double offset_ns = interval_ns * static_cast<double>(p)
                               / static_cast<double>(producers);
        auto &svc = service[p];
        auto &rsp = response[p];
        for (size_t k = 0; k < per_producer; ++k) {
          Clock::time_point scheduled;
          if (open_loop) {
            scheduled =
                start
                + std::chrono::nanoseconds(static_cast<int64_t>(
                    offset_ns + static_cast<double>(k / burst) * interval_ns));
            waitUntil(scheduled);
          }
          const auto t0 = Clock::now();
          out.push(message.stamp(++seq, t0));
          const auto t1 = Clock::now();
          svc.record(elapsedNs(t0, t1));
          rsp.record(elapsedNs(open_loop ? scheduled : t0, t1));
        }
        producer_cpu[p] = threadCpuNs() - cpu_start;
      });
    }

    // Warm-up events are written out before the measurement starts
    gate.waitReady();
    target->flush();

    measuring.store(true, std::memory_order_release);
    const auto cpu_start = processCpuNs();
    const auto start = gate.open();
    for (auto &t : threads) {
      t.join();
    }
    target->flush();  // Writing out the rest is part of the work
    const auto end = Clock::now();
    const auto cpu_end = processCpuNs();
    measuring.store(false, std::memory_order_release);

    if (reader.joinable()) {
      stop.store(true, std::memory_order_release);
      reader.join();
    }
    target.reset();

    result.seconds = std::chrono::duration<double>(end - start).count();
    for (size_t p = 0; p < producers; ++p) {
      result.service.merge(service[p]);
      result.response.merge(response[p]);
    }
    uint64_t foreground_cpu = reader_cpu;
    for (auto cpu : producer_cpu) {
      foreground_cpu += cpu;
    }
    const auto process_cpu = cpu_end - cpu_start;
    result.background_cpu =
        process_cpu > foreground_cpu
            ? static_cast<double>(process_cpu - foreground_cpu) / 1e9
                  / result.seconds
            : 0;

    if (verify) {
      if (not spec.track_delivery) {
        checker.feedFile(spec.path);
      }
      checker.finish(spec.warmup + per_producer);
      result.verified = true;
      result.ok = checker.ok();
      result.error = checker.error();
      result.lines = checker.lines();
      result.foreign_lines = checker.foreignLines();
      result.bytes = checker.bytes();
    }
    return result;
  }

  /// Target doing nothing: measures the overhead of the driver itself.
  struct NullTarget {
    void push(std::string_view message) noexcept {
      asm volatile("" : : "r"(message.data()), "r"(message.size()) : "memory");
    }
    void flush() noexcept {}
  };

}  // namespace bench

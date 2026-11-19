/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file compare_spdlog.cpp
 * @brief spdlog under the comparison load.
 *
 * --mode async (default): async_logger on a thread pool of one worker with
 *   a queue of `capacity` messages, overflow policy `block`;
 * --mode sync: plain logger, the producer writes the file itself under the
 *   sink mutex.
 *
 * Both write through a file sink equal to basic_file_sink_mt (stdio buffer),
 * which also counts flushes: async_logger::flush() only enqueues a request,
 * and the benchmark needs to know when the request has been carried out.
 * Latency is mapped to a periodic flush, like spdlog::flush_every(), done by
 * a thread of the adapter so that every flush request is accounted for.
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/version.h>

#include "compare_runner.hpp"

namespace {

  /// basic_file_sink_mt counting completed flushes.
  class CountingFileSink final : public spdlog::sinks::base_sink<std::mutex> {
   public:
    explicit CountingFileSink(const std::string &path) {
      file_.open(path, false);
    }

    uint64_t flushed() const {
      return flushed_.load(std::memory_order_acquire);
    }

   protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
      spdlog::memory_buf_t formatted;
      formatter_->format(msg, formatted);
      file_.write(formatted);
    }

    void flush_() override {
      file_.flush();
      flushed_.fetch_add(1, std::memory_order_release);
    }

   private:
    spdlog::details::file_helper file_;
    std::atomic<uint64_t> flushed_{0};
  };

  struct SpdlogTarget {
    SpdlogTarget(const bench::compare::Options &opt, bool async)
        : async_(async),
          sink_(std::make_shared<CountingFileSink>(opt.sink_path.string())) {
      if (async_) {
        pool_ = std::make_shared<spdlog::details::thread_pool>(opt.capacity, 1);
        logger_ = std::make_shared<spdlog::async_logger>(
            "benchmark", sink_, pool_, spdlog::async_overflow_policy::block);
      } else {
        logger_ = std::make_shared<spdlog::logger>("benchmark", sink_);
      }
      // Same line layout as soralog's file sink
      logger_->set_pattern("%y.%m.%d %H:%M:%S.%f  %-8l  %n  %v");
      logger_->set_level(spdlog::level::info);
      logger_->flush_on(spdlog::level::off);
      if (opt.latency_ms != 0) {
        const auto interval = std::chrono::milliseconds(opt.latency_ms);
        flusher_ = std::thread([this, interval] {
          std::unique_lock lock(mutex_);
          while (not stop_) {
            if (not cv_.wait_for(lock, interval, [this] { return stop_; })) {
              requestFlush();
            }
          }
        });
      }
    }

    ~SpdlogTarget() {
      {
        std::lock_guard lock(mutex_);
        stop_ = true;
      }
      cv_.notify_one();
      if (flusher_.joinable()) {
        flusher_.join();
      }
      logger_.reset();  // the pool joins its worker after the queue drains
      pool_.reset();
    }

    void push(std::string_view message) {
      logger_->info("{}", message);
    }

    /// Returns when everything pushed before is written and flushed.
    void flush() {
      uint64_t number = 0;
      {
        std::lock_guard lock(mutex_);
        number = requestFlush();
      }
      while (sink_->flushed() < number) {
        std::this_thread::yield();
      }
    }

   private:
    /// Requests a flush, called under mutex_: requests are numbered in the
    /// order they are queued, so the n-th completed flush is the n-th request.
    uint64_t requestFlush() {
      logger_->flush();
      return ++requested_;
    }

    const bool async_;
    std::shared_ptr<CountingFileSink> sink_;
    std::shared_ptr<spdlog::details::thread_pool> pool_;
    std::shared_ptr<spdlog::logger> logger_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
    uint64_t requested_ = 0;
    std::thread flusher_;
  };

}  // namespace

int main(int argc, char **argv) {
  const auto opt = bench::compare::parse(argc, argv);
  const bool async = opt.mode.empty() or opt.mode == "async";
  if (not async and opt.mode != "sync") {
    std::fprintf(stderr, "--mode for spdlog: async or sync\n");
    return 2;
  }
  const std::string version = std::to_string(SPDLOG_VER_MAJOR) + "."
                            + std::to_string(SPDLOG_VER_MINOR) + "."
                            + std::to_string(SPDLOG_VER_PATCH);
  const std::string settings =
      (async ? "async_logger, 1 worker, queue " + std::to_string(opt.capacity)
                   + " messages, overflow: block"
             : std::string("synchronous logger, file sink under mutex"))
      + "; stdio write buffer; periodic flush every "
      + std::to_string(opt.latency_ms)
      + " ms; message limit and write buffer size not applicable";
  return bench::compare::run(
      opt, async ? "spdlog-async" : "spdlog-sync", version, settings, [&] {
        return std::make_unique<SpdlogTarget>(opt, async);
      });
}

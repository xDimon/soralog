/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file compare_quill.cpp
 * @brief quill under the comparison load.
 *
 * quill keeps a queue per producer thread and formats on its backend thread.
 * The queue is made bounded and blocking, like the queue of soralog; its
 * size is a compile-time option of quill, so --capacity does not apply and
 * the queue is kQueueBytes per producer. Latency is mapped to the minimal
 * interval between flushes of the backend; --buffer-size to the write buffer
 * of the file sink.
 */

#include <chrono>
#include <memory>
#include <string>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/FileSink.h>

#include "compare_runner.hpp"

namespace {

  constexpr size_t kQueueBytes = 128u * 1024u;  // quill default

  struct FrontendOptions {
    static constexpr quill::QueueType queue_type =
        quill::QueueType::BoundedBlocking;
    static constexpr size_t initial_queue_capacity = kQueueBytes;
    static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
    static constexpr size_t unbounded_queue_max_capacity = kQueueBytes;
    static constexpr quill::HugePagesPolicy huge_pages_policy =
        quill::HugePagesPolicy::Never;
  };

  using Frontend = quill::FrontendImpl<FrontendOptions>;
  using Logger = quill::LoggerImpl<FrontendOptions>;

  struct QuillTarget {
    explicit QuillTarget(const bench::compare::Options &opt) {
      quill::FileSinkConfig config;
      config.set_open_mode('w');
      config.set_filename_append_option(quill::FilenameAppendOption::None);
      if (opt.buffer_size) {
        config.set_write_buffer_size(*opt.buffer_size);
      }
      auto sink = Frontend::create_or_get_sink<quill::FileSink>(
          opt.sink_path.string(), config, quill::FileEventNotifier{});
      // Same line layout as soralog's file sink
      logger = Frontend::create_or_get_logger(
          "benchmark",
          std::move(sink),
          quill::PatternFormatterOptions{
              "%(time)  %(log_level:<8)  %(logger)  %(message)",
              "%y.%m.%d %H:%M:%S.%Qus"});
    }

    ~QuillTarget() {
      // Waits until the backend has dropped the logger and its file sink
      Frontend::remove_logger_blocking(logger);
    }

    void push(std::string_view message) {
      LOG_INFO(logger, "{}", message);
    }

    void flush() {
      logger->flush_log();
    }

    Logger *logger = nullptr;
  };

}  // namespace

int main(int argc, char **argv) {
  const auto opt = bench::compare::parse(argc, argv);

  quill::BackendOptions backend;
  backend.sink_min_flush_interval = std::chrono::milliseconds(opt.latency_ms);
  // Producers waiting for room in a full queue is expected under load and
  // shows in the push times; other notifications are errors
  backend.error_notifier = [](const std::string &message) {
    if (message.find("blocking occurrences") == std::string::npos) {
      std::fprintf(stderr, "%s\n", message.c_str());
    }
  };
  quill::Backend::start(backend);

  const std::string version = std::to_string(quill::VersionMajor) + "."
                            + std::to_string(quill::VersionMinor) + "."
                            + std::to_string(quill::VersionPatch);
  const std::string
      settings = "queue per producer " + std::to_string(kQueueBytes)
               + " B, bounded blocking; 1 backend thread formats and writes; "
                 "flush not more often than every "
               + std::to_string(opt.latency_ms) + " ms; write buffer "
               + (opt.buffer_size ? std::to_string(*opt.buffer_size) + " B"
                                  : "default")
               + "; --capacity and --message-length not applicable";
  const int rc = bench::compare::run(opt, "quill", version, settings, [&] {
    return std::make_unique<QuillTarget>(opt);
  });

  quill::Backend::stop();
  return rc;
}

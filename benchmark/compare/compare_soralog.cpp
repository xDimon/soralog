/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file compare_soralog.cpp
 * @brief soralog under the comparison load: Logger -> SinkToFile, set up
 * manually through LoggingSystem, as an application would.
 */

#include <memory>
#include <string>

#include <soralog/impl/sink_to_file.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

#include "compare_runner.hpp"

namespace {

  struct SoralogTarget {
    explicit SoralogTarget(const bench::compare::Options &opt) {
      sink = system.makeSink<soralog::SinkToFile>(
          "file",
          soralog::Level::TRACE,
          opt.sink_path,
          soralog::Sink::ThreadInfoType::NONE,
          opt.capacity,
          opt.message_length,
          opt.buffer_size,
          opt.latency_ms);
      system.makeGroup("benchmark", {}, "file", soralog::Level::INFO);
      logger = system.getLogger("benchmark", "benchmark");
    }

    void push(std::string_view message) {
      logger->info("{}", message);
    }

    void flush() {
      sink->flush();
    }

    soralog::LoggingSystem system;
    std::shared_ptr<soralog::SinkToFile> sink;
    std::shared_ptr<soralog::Logger> logger;
  };

}  // namespace

int main(int argc, char **argv) {
  const auto opt = bench::compare::parse(argc, argv);
  const std::string settings =
      "event queue " + std::to_string(opt.capacity) + " events, message limit "
      + std::to_string(opt.message_length) + " B, write buffer "
      + (opt.buffer_size ? std::to_string(*opt.buffer_size) + " B" : "default")
      + ", latency " + std::to_string(opt.latency_ms)
      + " ms; full queue: producer waits";
  return bench::compare::run(opt, "soralog", SORALOG_VERSION, settings, [&] {
    return std::make_unique<SoralogTarget>(opt);
  });
}

[//]: # (
Copyright Soramitsu Co., 2021-2023
Copyright Quadrivium Co., 2023
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Soralog: High-Performance Logging Library

## Overview

Soralog is a high-performance, flexible logging library designed for C++ applications. It provides robust logging features, supports multiple logging backends, and allows extensive customization. The library is designed to be lightweight, efficient, and easy to integrate into various projects.

## Features

- Multiple log sinks: console, file, syslog, and more.
- Efficient circular buffer for log messages, without locks.
- Thread-safe logging with minimal overhead.
- Hierarchical logging groups with inheritance.
- Configurable log levels and formats.
- Configurable via YAML or programmatically.
- Macro-based or object-based logging interface (up to user).

## Performance

Typical numbers for a file sink with default settings (event buffer of 2048
events, message length limit 1024 bytes, write buffer 4 MB, latency 1000 ms),
64-byte messages, 112-byte lines. Release build, Apple M4 Max (12 performance
+ 4 efficiency cores), macOS 27.2, APFS on the internal SSD, Homebrew clang 23.
Median of 5 runs of 2,000,000 events each.

| Writer threads | Throughput, events/s | `push` p50 | `push` p99 | `push` p99.9 |
|---:|---:|---:|---:|---:|
| 1 | 15 M | < 0.05 µs | 0.09 µs | 0.17 µs |
| 2 | 11 M | 0.17 µs | 0.34 µs | 0.54 µs |
| 4 | 9–10 M | 0.30 µs | 1.3–1.5 µs | 2.2–2.4 µs |
| 8 | 4.4 M | 0.93 µs | 13 µs | 27 µs |

Throughput here is the saturation point: writers log as fast as they can and
the sink worker writing the file is busy all the time; past it writers wait
for free room in the buffer. At a steady 100,000 events/s the worker takes
under 1% of a core, `push` stays at p99.9 below 1 µs (1 thread) or 3.4 µs
(4 threads), and events reach the file within 17 ms: with this load the
worker is woken by the buffer filling up rather than by the latency timer.

Numbers depend on the machine, the file system and the settings; the clock of
this machine ticks every 41.7 ns, so the shortest times are rounded to it. To
measure on your hardware build with `-DBUILD_BENCHMARKS=ON` and run
`soralog_benchmark --scenario load --sink-path <file>`; see its `--help` for
the load and sink parameters. The method and the comparison with other
loggers are in [docs/benchmark](docs/benchmark/README.md).

## Installation

### Dependencies

Soralog requires:
- `fmt` for formatting log messages.
- `yaml-cpp` for convenient configuring.
- `gtest` for unit tests (optional).

Soralog uses CMake as its build system and supports different configurations through CMake options.

### Build Options

- `BUILD_TESTS` (default: `OFF`) – Build unit tests.
- `EXAMPLES` (default: `OFF`) – Build example programs; enable with `-DEXAMPLES=ON`.
- `BUILD_BENCHMARKS` (default: `OFF`) – Build the benchmark of the event buffer and sinks.
- `BUILD_BENCHMARK_COMPARE` (default: `OFF`) – Build the comparison with other loggers (spdlog, quill); needs vcpkg, implies `BUILD_BENCHMARKS`.
- `ASAN`, `LSAN`, `MSAN`, `TSAN`, `UBSAN` (default: `OFF`) – Enable sanitizers.

### Supported Package Managers

Soralog supports dependency management via `hunter` and `vcpkg`. By default, `hunter` is used unless a `vcpkg` toolchain file is detected.

```sh
git clone https://github.com/xDimon/soralog.git
cd soralog
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON -DEXAMPLES=ON
make
```

To use `vcpkg`, specify the toolchain file:

```sh
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg.cmake ..
```

To include Soralog in your project, add the following to your CMakeLists.txt:

```cmake
find_package(soralog REQUIRED)
target_link_libraries(your_project PRIVATE soralog::soralog)
```

## Basic Usage

### Creating a Logger

Before using any loggers, you must configure the logging system. The `LoggingSystem::configure()` method must be called, returning a `Configurator::Result` indicating success or failure.

```cpp
LoggingSystem logging_system(configurator); // See examples
const auto result = logging_system.configure();
if (result.has_error) {
  std::cerr << result.message;
}
auto logger = logging_system.getLogger("logger", "main_group");  // std::shared_ptr<soralog::Logger>
```

### Using logger methods

```cpp
logger->debug("Hello, {}!", "world");
logger->info("Hello, {}!", "world");
```

### Using Logging Macros

```cpp
SL_INFO(logger, "Hello, {}!", "world");
SL_WARN(logger, "Hello, {}!", "world");
```

## Configuration model

Soralog supports passing multiple configurators to the `LoggingSystem`. These configurators are applied sequentially, allowing flexible and incremental configuration.

The configuration process occurs in four waves:

1. **Prepare** – initial setup and validation.
2. **Apply sinks** – creation and configuration of sinks.
3. **Apply groups** – setup of logging groups with hierarchy.
4. **Cleanup** – finalization and resource management.

Later configurators may override or extend settings from earlier ones, enabling complex configurations via cascading.

## Configuration File

Soralog uses a YAML-based configuration file. Below is a detailed breakdown of all available parameters.

### Notes

- The builtin sink `"*"` always exists and discards all output.
- The `syslog` sink is process-global; only one sink of `type: syslog` may be defined.
- The `latency` parameter controls flushing delay in milliseconds. Setting `latency: 0` makes every log call write its event out before returning, which may increase overhead.

### Example YAML Configuration

```yaml
sinks:
  - name: colored_stdout
    type: console
    stream: stdout
    color: true

  - name: file
    type: file
    path: /tmp/soralog.log

groups:
  - name: main
    sink: colored_stdout
    level: trace
    is_fallback: true
    children:
      - name: example_group
```

### Sink Configuration Options

#### Common Sink Parameters

| Parameter        | Type    | Required | Default    | Description                                                                                                 |
|------------------|---------|----------|------------|-------------------------------------------------------------------------------------------------------------|
| `name`           | string  | Yes      | N/A        | Unique identifier of the sink                                                                               |
| `type`           | string  | Yes      | N/A        | Sink type (`console`, `file`, `syslog`, `multisink`)                                                        |
| `thread`         | string  | No       | `none`     | Thread info mode (`name`, `id`, `none`)                                                                     |
| `capacity`       | int     | No       | `64` / `2048`           | Number of buffered events (console / file and syslog)                                          |
| `max_message_length` | int | No       | `1024`                  | Max length of a log message in bytes, at least 64                                              |
| `buffer`         | int     | No       | `131072` / `4194304`    | Size of the write buffer in bytes (console / file and syslog)                                  |
| `latency`        | int     | No       | `200` / `1000`          | Max delay in milliseconds before flushing (console / file and syslog). `0` means synchronous flush on every log event (higher overhead). |
| `level`          | string  | No       | `trace`    | Minimum log level (`trace`, `debug`, `verbose`, `info`, `warn`, `error`, `critical`)                         |

Events are written out by a worker thread at least every `latency` milliseconds, and earlier when the event buffer is 80% full. With `latency: 0` every log call writes its event out before returning.

#### Console Sink (`type: console`)

| Parameter | Type   | Required | Default  | Description                         |
|-----------|--------|----------|----------|-------------------------------------|
| `stream`  | string | No       | `stdout` | Output stream (`stdout`, `stderr`)  |
| `color`   | bool   | No       | `false`  | Enables colored output              |

#### File Sink (`type: file`)

| Parameter  | Type   | Required | Default | Description                                       |
|------------|--------|----------|---------|---------------------------------------------------|
| `path`     | string | Yes      | N/A     | Path to the log file                              |
| `at_fault` | string | No       | `wait`  | Action on failure (`terminate`, `wait`, `ignore`) |

#### Syslog Sink (`type: syslog`)

| Parameter | Type   | Required | Default | Description                    |
|-----------|--------|----------|---------|--------------------------------|
| `ident`   | string | No       | N/A     | Identifier for syslog messages |

#### Multisink (`type: multisink`)

| Parameter | Type  | Required | Default | Description                           |
|-----------|-------|----------|---------|---------------------------------------|
| `sinks`   | array | Yes      | N/A     | List of sink names to forward logs to |

### Group Configuration Options

| Parameter     | Type   | Required              | Default | Description                                                                          |
|---------------|--------|-----------------------|---------|--------------------------------------------------------------------------------------|
| `name`        | string | Yes                   | N/A     | Unique identifier of the group                                                       |
| `sink`        | string | Yes                   | N/A     | Sink used for this group                                                             |
| `level`       | string | Yes (for root groups) | N/A     | Minimum log level (`trace`, `debug`, `verbose`, `info`, `warn`, `error`, `critical`) |
| `is_fallback` | bool   | No                    | `false` | Marks this group as the default fallback                                             |
| `children`    | array  | No                    | `[]`    | List of child groups                                                                 |

## FATAL / CRITICAL Semantics

- **CRITICAL** log level causes the current sink to flush immediately.
- **FATAL** log level flushes all sinks and then aborts the process.

## Examples

Soralog provides various examples demonstrating different use cases and configurations. Below is a list of available examples:

1. **01-hello_world** – A minimal example that initializes a logger and writes a simple message.
2. **02-manual** – Demonstrates programmatic configuration of logging without using a YAML config file.
3. **03-with_config_file** – Shows how to configure Soralog using a YAML configuration file.
4. **04-multi_config** – Chains configurators: a later one overrides settings of a group declared by an earlier one.
5. **05-two_sinks** – Example of logging to two different sinks (e.g., console and file).
6. **06-multisink** – Uses a multisink to send log messages to multiple destinations.
7. **07-multisink_with_different_level** – Extends the multisink example by setting different log levels for different sinks.
8. **08-logfile** – Logging to file with buffering and latency control.
9. **09-logfile_rotate** – Logfile rotation using `logrotate`.
10. **99-most_features** – A comprehensive example showcasing most of Soralog’s features, including:
    - multi configurators
    - multisinks
    - per-sink log levels
    - thread naming and concurrent logging
    - long message truncation

### Example Source Code

The source code for these examples is available in the [example](https://github.com/xDimon/soralog/tree/master/example) directory.

## Projects Using Soralog

Soralog is utilized in several open-source projects, including:

- [cpp-libp2p](https://github.com/libp2p/cpp-libp2p)
- [qlean-mini](https://github.com/qdrvm/clean-mini)
- [Kagome](https://github.com/qdrvm/kagome)
- [cpp-jam](https://github.com/qdrvm/cpp-jam)

## Contributing

We welcome contributions! Feel free to submit issues and pull requests.

## License

Soralog is licensed under the Apache-2.0 license. See the LICENSE file for details.

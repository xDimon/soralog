[//]: # (
Copyright Soramitsu Co., 2021-2023
Copyright Quadrivium Co., 2023
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Soralog: High-Performance Logging Library

[Русская версия](README.ru.md)

## Overview

Soralog is a high-performance, flexible logging library designed for C++ applications. It provides robust logging features, supports multiple logging backends, and allows extensive customization. The library is designed to be lightweight, efficient, and easy to integrate into various projects.

## Features

- Multiple log sinks: console, file, syslog, Windows Event Log, and more.
- Efficient circular buffer for log messages, without locks.
- Thread-safe logging with minimal overhead.
- Hierarchical logging groups with inheritance.
- Configurable log levels and thread info (name or id) in the lines.
- Configurable via YAML or programmatically.
- Macro-based or object-based logging interface (up to user).
- Platforms: Linux, macOS, Windows (MSVC, MinGW), FreeBSD 14 and 15, OpenBSD 7.8,
  NetBSD 10 and DragonFly BSD 6.4; static or shared libraries.

## Why Soralog

### Performance

Measured against spdlog and quill under the same load, with the same line
layout and nobody dropping events
([method](docs/benchmark/README.md),
[full results](docs/benchmark/results/apple-m4-max-macos-27.2/report.md);
Apple M4 Max):

- **Highest throughput.** From one thread Soralog writes 15 M events/s,
  about 3× quill and 3.5–4.5× spdlog; from 4 threads it writes 8.3 M, where
  quill does 4.6 M and spdlog 1.2–1.9 M.
- **Cheap logging calls.** At a steady 100,000 events/s a call takes 0.04 µs
  at the median and stays under 0.6 µs at p99.9; spdlog has tails
  of 6–13 µs.
- **Little CPU in the background.** At 100,000 events/s the worker takes
  under 1% of a core, against ~6% for quill's backend and ~25% for spdlog's async
  worker.
- **Freshness is a setting.** How fresh the file is depends on `latency`:
  with 1–10 ms lines reach the file within ~1.5–13 ms, and the calls cost
  the same ([how settings affect
  performance](docs/benchmark/soralog-settings.md)).
- **Bounded memory.** The event queue is allocated once (2.25 MiB by
  default) and never grows. When it is full, writers wait for room, so no
  event is lost.

### Convenience

- **Hierarchical groups.** Loggers belong to groups, and groups form a tree:
  a group takes its level and sink from its parent unless it sets its own.
  Changing a group's level or sink at runtime applies to every group and
  logger below it that does not override it.
- **Configuration as data.** A YAML file, or several applied in order,
  describes sinks and groups. The same can be done in code, and both ways
  can be combined.
- **Sinks for the usual destinations.** They cover console (optionally
  colored), file (reopened on request, for external log rotation), syslog,
  Windows Event Log, and multisinks fanning out to several of them. Each
  sink has its own level and buffering.
- **Details that save time.** Lines can carry thread names or ids, and
  messages use `fmt`-style formatting. A sink reacts to a failing disk as
  configured: it waits, ignores or terminates the process. Trace and debug
  calls can be compiled out entirely.

### When to choose it

Soralog fits best where an application logs a lot from a few threads and
must not slow down for it: servers, network nodes, processing pipelines,
anything where logging sits on the hot path. There its throughput is
higher and its background load lower than those of spdlog and quill, and
its configuration can be changed without a restart.

Look at the alternatives in three cases.

1. **Many threads log at saturation.** quill keeps a queue per thread: with
   4 threads its calls stay at ~0.2 µs at p99.9, while Soralog's reach
   ~3 µs. With 8 threads both do ~4.4 M events/s.
2. **Lines must reach the file within a millisecond without any tuning.**
   spdlog's small stdio buffer delivers them in ~0.5 ms at 100,000 events/s,
   paid for with microsecond tails in the calls. Soralog needs `latency: 1`
   for that.
3. **A custom line layout or ready-made sinks for every case are needed**,
   such as rotation by size or date or network output: spdlog has more of
   them.

## Performance

Typical numbers for a file sink with default settings (event buffer of 2048
events, message length limit 1024 bytes, write buffer 4 MB, latency 1000 ms),
64-byte messages, 112-byte lines, logged through `Logger`. Release build,
Apple M4 Max (12 performance + 4 efficiency cores), macOS 27.2, APFS on the
internal SSD, Homebrew clang 23. Median of 3 runs of 1,000,000 events each.

| Writer threads | Throughput, events/s | `push` p50 | `push` p99 | `push` p99.9 |
|---:|---:|---:|---:|---:|
| 1 | 15.0 M | < 0.05 µs | 0.09 µs | 0.22 µs |
| 2 | 11.1 M (9.7–11.3 M) | 0.17 µs | 0.43 µs | 0.64 µs |
| 4 | 8.3 M (6.3–8.7 M) | 0.43 µs | 1.7 µs | 2.8 µs |
| 8 | 4.4 M | 0.86 µs | 15 µs | 33 µs |

Throughput here is the saturation point. Writers log as fast as they can,
the sink worker is busy writing the file all the time, and past that point
writers wait for room in the buffer.

At a steady 100,000 events/s the worker takes under 1% of a core, and
`push` stays at p99.9 below 0.6 µs with 1 thread or 3.4 µs with 4. Events
reach the file within 20 ms: at this load the worker is woken by the
buffer filling up, before the latency timer fires.

Numbers depend on the machine, the file system and the settings; the clock of
this machine ticks every 41.7 ns, so the shortest times are rounded to it. To
measure on your hardware build with `-DBUILD_BENCHMARKS=ON` and run
`soralog_benchmark --scenario load --sink-path <file>`; see its `--help` for
the load and sink parameters. The method, the comparison with other loggers,
its results and how the settings affect performance are in
[docs/benchmark](docs/benchmark/README.md).

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
- `BUILD_SHARED_LIBS` (default: `OFF`) – Build shared libraries instead of static ones; on Windows the DLLs export all their functions.
- `SYSLOG_SINK` (default: `ON`, `OFF` on Windows) – Build the syslog sink; not available on Windows.
- `EVENTLOG_SINK` (default: `ON` on Windows, `OFF` elsewhere) – Build the Windows Event Log sink; available on Windows only.
- `BUILD_BENCHMARKS` (default: `OFF`) – Build the benchmark of the event buffer and sinks.
- `BUILD_BENCHMARK_COMPARE` (default: `OFF`) – Build the comparison with other loggers (spdlog, quill); needs vcpkg, implies `BUILD_BENCHMARKS`.
- `ASAN`, `LSAN`, `MSAN`, `TSAN`, `UBSAN` (default: `OFF`) – Enable sanitizers.

### Supported Package Managers

Soralog supports dependency management via `hunter` and `vcpkg`, or takes dependencies already installed (`PACKAGE_MANAGER=none`). By default, `hunter` is used unless a `vcpkg` toolchain file is detected.

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

To build without a package manager, set `PACKAGE_MANAGER=none`; `fmt` and `yaml-cpp` (and `GTest` for tests) must then be found by `find_package` in config mode, e.g. through `CMAKE_PREFIX_PATH`:

```sh
cmake -DPACKAGE_MANAGER=none -DCMAKE_PREFIX_PATH=/path/to/deps ..
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

On Windows `<windows.h>` defines macros `ERROR` and `IGNORE`. Soralog headers
and macros work with either include order, but `Level::ERROR`,
`Level::IGNORE` and `AtFaultReactionType::IGNORE` written in your own code
after `<windows.h>` don't compile: include `<windows.h>` after them or
`#undef ERROR` and `#undef IGNORE`.

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
| `type`           | string  | Yes      | N/A        | Sink type (`console`, `file`, `syslog`, `eventlog`, `multisink`)                                            |
| `thread`         | string  | No       | `none`     | Thread info mode (`name`, `id`, `none`)                                                                     |
| `capacity`       | int     | No       | `64` / `2048`           | Number of buffered events (console / file, syslog and eventlog)                                |
| `max_message_length` | int | No       | `1024`                  | Max length of a log message in bytes, at least 64                                              |
| `buffer`         | int     | No       | `131072` / `4194304`    | Size of the write buffer in bytes (console / file, syslog and eventlog)                        |
| `latency`        | int     | No       | `200` / `1000`          | Max delay in milliseconds before flushing (console / file, syslog and eventlog). `0` means synchronous flush on every log event (higher overhead). |
| `level`          | string  | No       | `trace`    | Minimum log level (`trace`, `debug`, `verbose`, `info`, `warn`, `error`, `critical`)                         |

Events are written out by a worker thread about every `latency` milliseconds, and earlier when the event buffer is 80% full. With small values the real period is longer than the setting: about 1.25× at 10 ms and 1.5× at 1 ms. With `latency: 0` every log call writes its event out before returning.

With many threads logging at once, the `capacity` matters for throughput too. On macOS with at least 16 hardware threads, a buffer of at least 8 events per hardware thread (128 with 16 threads) lets the logging threads claim slots without contending for them: measured on Apple M4 Max with the default 2048, 1.7–4.7 times the events per second with 4–16 threads logging as fast as they can; the gain is smaller with a smaller buffer. Below 8 events per hardware thread, and on other systems, the threads keep the contended way, which is safer when the threads outnumber the cores.

#### Console Sink (`type: console`)

| Parameter | Type   | Required | Default  | Description                         |
|-----------|--------|----------|----------|-------------------------------------|
| `stream`  | string | No       | `stdout` | Output stream (`stdout`, `stderr`)  |
| `color`   | bool   | No       | `false`  | Enables colored output              |

On Windows a colored sink switches the console to the virtual terminal mode;
if the console doesn't support it, the output goes without colors.

#### File Sink (`type: file`)

| Parameter  | Type   | Required | Default | Description                                       |
|------------|--------|----------|---------|---------------------------------------------------|
| `path`     | string | Yes      | N/A     | Path to the log file, UTF-8                       |
| `at_fault` | string | No       | `wait`  | Action on failure (`terminate`, `wait`, `ignore`) |

#### Syslog Sink (`type: syslog`)

| Parameter | Type   | Required | Default | Description                    |
|-----------|--------|----------|---------|--------------------------------|
| `ident`   | string | Yes      | N/A     | Identifier for syslog messages |

#### Event Log Sink (`type: eventlog`, Windows)

| Parameter  | Type    | Required | Default | Description                                  |
|------------|---------|----------|---------|----------------------------------------------|
| `source`   | string  | Yes      | N/A     | Event source name, as Event Viewer shows it  |
| `event_id` | integer | No       | `0`     | Event ID of the records, 0 to 65535          |

Records go to the Application log: fatal, critical and error as errors,
warnings as warnings, info, verbose and debug as information; trace is not
logged. The sink registers nothing, so Event Viewer prefixes the text with a
note that the description of the event ID is not found. To drop the note, the
installer of the application registers the source with a message file whose
message of that event ID is `%1`.

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

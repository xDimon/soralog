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
- Efficient lock-free circular buffer for log messages.
- Thread-safe logging with minimal overhead.
- Hierarchical logging groups with inheritance.
- Configurable log levels and formats.
- Configurable via YAML or programmatically.
- Macro-based or object-based logging interface (up to user).

## Installation

### Dependencies

Soralog requires:
- `fmt` for formatting log messages.
- `yaml-cpp` for convenient configuring.
- `gtest` for unit tests (optional).

Soralog uses CMake as its build system and supports different configurations through CMake options.

### Build Options

- `BUILD_TESTS` (default: `OFF`) – Build unit tests.
- `EXAMPLES` (default: `OFF`) – Build example programs.
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
target_link_libraries(your_project PRIVATE soralog)
```

## Basic Usage

### Creating a Logger

```cpp
    LoggingSystem logging_system(configurator); // See examples
    soralog::Logger logger = logging_system.getLogger("loggger", "main_group");
}
```

### Using logger methods

```cpp
    logger.debug("Hello, {}!", "world");
    logger.info("Hello, {}!", "world");
```

### Using Logging Macros

```cpp
    SL_INFO(logger, "Hello, {}!", "world");
    SL_WARN(logger, "Hello, {}!", "world");
```

## Configuration File

Soralog uses a YAML-based configuration file. Below is a detailed breakdown of all available parameters.

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

| Parameter        | Type    | Required | Default    | Description                                                                          |
|------------------|---------|----------|------------|--------------------------------------------------------------------------------------|
| `name`           | string  | Yes      | N/A        | Unique identifier of the sink                                                        |
| `type`           | string  | Yes      | N/A        | Sink type (`console`, `file`, `syslog`, `multisink`)                                 |
| `thread`         | string  | No       | `none`     | Thread info mode (`name`, `id`, `none`)                                              |
| `capacity`       | int     | No       | `64`       | Max lock-free buffered messages                                                      |
| `buffer`         | int     | No       | `131072`   | Max buffer size in bytes                                                             |
| `latency`        | int     | No       | `100`      | Max delay in milliseconds before flushing                                            |
| `level`          | string  | No       | `trace`    | Minimum log level (`trace`, `debug`, `verbose`, `info`, `warn`, `error`, `critical`) |

#### Console Sink (`type: console`)

| Parameter | Type   | Required | Default  | Description                        |
|-----------|--------|----------|----------|------------------------------------|
| `stream`  | string | No       | `stdout` | Output stream (`stdout`, `stderr`) |
| `color`   | bool   | No       | `false`  | Enables colored output             |

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

## Examples

## Examples

Soralog provides various examples demonstrating different use cases and configurations. Below is a list of available examples:

1. **01-hello_world** – A minimal example that initializes a logger and writes a simple message.
2. **02-manual** – Demonstrates programmatic configuration of logging without using a YAML config file.
3. **03-with_config_file** – Shows how to configure Soralog using a YAML configuration file.
4. **04-cascade_config** – Demonstrates hierarchical logging groups with inherited settings.
5. **05-two_sinks** – Example of logging to two different sinks (e.g., console and file).
6. **06-multisink** – Uses a multisink to send log messages to multiple destinations.
7. **07-multisink_with_different_level** – Extends the multisink example by setting different log levels for different sinks.
8. **99-most_features** – A comprehensive example showcasing most of Soralog’s features, including advanced configuration, multi-threading, and custom loggers.

### Example Source Code

The source code for these examples is available in the [example](https://github.com/xDimon/soralog/tree/main/example) directory.

## Projects Using Soralog

Soralog is utilized in several open-source projects, including:

- [cpp-libp2p](https://github.com/libp2p/cpp-libp2p)
- [Kagome](https://github.com/qdrvm/kagome)
- [cpp-jam](https://github.com/qdrvm/cpp-jam)

## Contributing

We welcome contributions! Feel free to submit issues and pull requests.

## License

Soralog is licensed under the Apache-2.0 license. See the LICENSE file for details.


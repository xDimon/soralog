[//]: # (
Copyright Soramitsu Co., 2021-2023
Copyright Quadrivium Co., 2023
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Adding a logger to the comparison

The comparison runs every logger under the same load generator, so adding
one means writing a small adapter and registering it. The load, the
measurements, the output check and the reports come for free. Read the
[method](README.md) first: the rules of a fair comparison there are what
the adapter has to keep.

## 1. Get the dependency

Add the logger to the `soralog-benchmark-compare` feature in `vcpkg.json`, so
that its version is pinned by the vcpkg baseline of the project like the
others:

```json
"soralog-benchmark-compare": {
  "description": "Comparison of soralog with other loggers in benchmarks",
  "dependencies": ["spdlog", "quill", "<your-logger>"]
}
```

A logger that is not in vcpkg can be added through `FetchContent` in
`benchmark/compare/CMakeLists.txt`; pin an exact tag and record the version
the adapter reports (see below).

## 2. Write the adapter

Create `benchmark/compare/compare_<name>.cpp`. The adapter is a type with
two calls and a destructor:

| Member | Must do |
|---|---|
| constructor | set up the logger writing to `opt.sink_path`, with the settings mapped from `opt` |
| `void push(std::string_view message)` | log the message at info level, formatted as `"{}"` (or the logger's equivalent), as an application would |
| `void flush()` | return only when everything logged before is **readable from the file**; a flush that only enqueues a request is not enough |
| destructor | finish the output and close the file; the file is checked after it |

A skeleton:

```cpp
#include "compare_runner.hpp"
#include <your/logger.h>

namespace {

  struct Target {
    explicit Target(const bench::compare::Options &opt) {
      // open opt.sink_path, queue of opt.capacity events, flush interval of
      // opt.latency_ms, a full queue blocks the writer, line layout as below
    }

    void push(std::string_view message) {
      // log at info level: "{}", message
    }

    void flush() {
      // write out and flush the file; wait until it is done
    }
  };

}  // namespace

int main(int argc, char **argv) {
  const auto opt = bench::compare::parse(argc, argv);
  const std::string settings = "how the settings are mapped, in one line";
  return bench::compare::run(opt, "<name>", "<version>", settings, [&] {
    return std::make_unique<Target>(opt);
  });
}
```

Make the comparison fair:

- **Block, don't drop.** When the queue is full the writer must wait. A
  dropping logger fails the output check; an unbounded queue hides the cost
  in memory and is not comparable either.
- **Same line layout** as the soralog file sink:
  `YY.MM.DD HH:MM:SS.uuuuuu  LEVEL     name  message`, level padded to 8,
  logger name `benchmark`, two spaces between fields. The B/line column of
  the result must match the other loggers.
- **Map the settings that exist, say which don't.** Queue capacity in
  events (`opt.capacity`), the longest delay before writing out
  (`opt.latency_ms`), the write buffer (`opt.buffer_size`, if set), the
  message length limit (`opt.message_length`). The `settings` line printed
  with every run says what was mapped and how; copy it into `SETTINGS` of
  the runner script.
- **No fsync**, the same as the others: the comparison measures writing into
  the page cache.
- **Report the version** the program was built with, from the logger's own
  version macros or constants, not a hand-written string.
- **Process-wide state.** If the logger has a singleton backend, start it
  once in `main` and make each `Target` create and remove its own logger and
  file sink, since the runner creates a new target for every run and
  removes the file in between (see `compare_quill.cpp`).
- **Silence expected notices.** Messages the logger prints about a full
  queue are expected under load; drop them, keep the errors.

Existing adapters to copy from: `compare_spdlog.cpp` (a synchronous and an
asynchronous mode, a flush that waits), `compare_quill.cpp` (a singleton
backend, compile-time queue options).

## 3. Register it

In `benchmark/compare/CMakeLists.txt`:

```cmake
find_package(<your-logger> CONFIG REQUIRED)
add_executable(compare_<name> compare_<name>.cpp)
target_link_libraries(compare_<name> <your-logger-target>)
```

In `benchmark/compare/run_compare.py`, add the executable to `LOGGERS`
(with extra arguments, if it has modes) and the description of the mapped
settings to `SETTINGS`.

## 4. Check it before measuring

Build Release with `-DBUILD_BENCHMARK_COMPARE=ON`, then:

```sh
# output check passes, B/line equals soralog's
build/benchmark/compare/compare_<name> --threads 1,4 --ops 300000 \
    --repeat 1 --sink-path /tmp/compare.log
# flush really waits: delivery is tracked and every event is found
build/benchmark/compare/compare_<name> --threads 2 --rate 100000 \
    --duration 1 --delivery --repeat 1 --sink-path /tmp/compare.log
# the whole quick suite, next to the other loggers
benchmark/compare/run_compare.py run --build-dir build --suite quick \
    --out /tmp/compare-quick
```

A run that prints `FAILED` has lost, duplicated or reordered events, or the
flush returned before the file had them; fix the adapter before looking at
the numbers.

Then run the `standard` suite as described in
[Running it on your hardware](README.md#running-it-on-your-hardware).

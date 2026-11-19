[//]: # (
Copyright Soramitsu Co., 2021-2023
Copyright Quadrivium Co., 2023
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Benchmarking soralog

How the performance of soralog is measured, how it compares with other
loggers, and how to repeat the measurements on your own hardware.

- [What is measured](#what-is-measured)
- [How the load is generated](#how-the-load-is-generated)
- [What the numbers depend on](#what-the-numbers-depend-on)
- [Comparison with other loggers](#comparison-with-other-loggers)
  ([adding a logger](adding-a-logger.md))
- [Running it on your hardware](#running-it-on-your-hardware)
- [Pitfalls](#pitfalls)

## What is measured

A logger costs the application in two places: the logging call in the
writer thread, and the work done later to get the line into the file. Both
are measured, for every event:

| Metric | Meaning |
|---|---|
| **throughput** | events per second, from the start of the run until the final flush has written everything out |
| **push** | duration of the logging call in the writer thread |
| **resp** (response) | from the moment the event was *scheduled* to the end of the logging call; differs from push when a writer falls behind its schedule (open loop only, see below) |
| **dlv** (delivery) | from the logging call to the moment the line can be read from the file |
| **bg CPU** | CPU time of the logger's own threads (process CPU time minus the writers and the file reader), in percent of one core |
| **B/line** | bytes per line in the file; equal for all loggers, so they write the same amount of data |

Latencies are kept in log-linear histograms (16 buckets per power of two,
at most ~6% above the true value) and reported as percentiles over all
events of all repeats; throughput and bg CPU as the median of the repeats.

Every run verifies the output: each event of each writer is in the file,
once, in the order it was logged. A logger dropping or reordering events
fails the run, so its numbers are not counted as a valid result.

## How the load is generated

The load generator (`benchmark/load_driver.hpp`) knows nothing about a
particular logger. Each logger is plugged in by an adapter with two calls:
`push(message)` and `flush()`, where `flush()` returns when everything logged
before is readable from the file.

- **Closed loop** (`--rate 0`, the default): every writer logs as fast as it
  can. The result is the *saturation* throughput: the point where the
  logger's own writing is the bottleneck and writers wait for it.
- **Open loop** (`--rate N`): events are scheduled at N per second in total,
  evenly split among the writers, optionally in bursts (`--burst`). A writer
  waits for the scheduled moment of each event, logs it, and moves on. If a
  call takes longer than the gap to the next event, the next events are
  late, and their response time includes the delay. Measuring from the
  scheduled moment rather than from the actual start of the call is what
  avoids *coordinated omission*: otherwise the events that should have been
  logged while a writer was stuck would never be measured, and the tail
  would look much better than the application sees it.

Every message starts with a 36-byte header: writer, sequence number and the
moment of the logging call; the rest is padding up to the message size.
A reader thread (`--delivery`) follows the growing file, parses the headers
and records the delivery time. Its resolution is the poll interval
(`--poll-us`, 100 µs by default), so delivery below a few hundred
microseconds is not resolved.

Before the measured events each writer logs a warm-up (as many events in
total as the queue holds), which is written out before the clock starts:
memory of the queue is touched, the file is open, threads are running.

The overhead of the generator itself (clock reads, stamping the header) is
measured on an empty target and printed with every run; push times include
it.

## What the numbers depend on

Settings of the file sink (`SinkToFile`):

| Setting | Option | Effect |
|---|---|---|
| event queue capacity | `--capacity` | how many events can wait to be written; with `latency` it sets how often the worker wakes up |
| message length limit | `--message-length` | longer messages are cut; each queue slot takes the limit plus 112 bytes, rounded up to 128 (1152 B for the default 1024), so the default queue takes 2.25 MiB |
| write buffer | `--buffer-size` | lines are collected here and written to the file in one call |
| latency | `--latency` | the longest time an event may wait before being written; 0 makes every logging call write its event synchronously |

Load:

| Factor | Option |
|---|---|
| number of writer threads | `--threads 1,2,4,8` (one row per value) |
| rate, events per second in total | `--rate` (0 for closed loop) |
| burstiness | `--burst` events back to back per tick of each writer |
| message size | `--message-size` |

The worker of the file sink wakes up when the latency timer expires *or*
when the queue is 80% full, whichever comes first. At a rate λ the queue
fills to 80% in `0.8 × capacity / λ`, so under steady load what matters is
the ratio `λ × latency / capacity`: when it is above 0.8 the latency setting
no longer applies and delivery is bounded by the queue filling up. For
example, with 2048 events and 100,000 events/s the worker wakes every ~16 ms
regardless of a latency of 1000 ms.

The number of writers matters in two ways, and the suites look at both:
at saturation (more writers contend for the queue: `saturation`) and at
a fixed total rate split among more writers (the contention itself,
without more load: `writers-500k`).

`soralog_benchmark --scenario load` takes all of these options for soralog
alone, including those the comparison keeps at defaults.

## Comparison with other loggers

`benchmark/compare` builds one executable per logger on the same load
generator, with the same options, output table and CSV:

| Executable | Logger | Setup |
|---|---|---|
| `compare_soralog` | soralog | `Logger` → `SinkToFile`, configured through `LoggingSystem` as an application would |
| `compare_spdlog --mode async` | spdlog | `async_logger`, one worker, queue of `capacity` messages, overflow policy `block` |
| `compare_spdlog --mode sync` | spdlog | synchronous logger: the writer formats and writes the file under the sink mutex; the baseline of not having a background thread at all |
| `compare_quill` | quill | per-thread queue, bounded and blocking, 128 KiB (a compile-time option of quill); the backend thread formats and writes |

The rules that keep the comparison fair:

- **Nobody drops events.** Every logger is set to make the writer wait when
  its queue is full; the output check would catch a logger that drops.
- **Same line.** The line layout of every logger is set to that of soralog
  (`YY.MM.DD HH:MM:SS.uuuuuu  LEVEL     name  message`); B/line confirms it.
- **Same file sink semantics.** Lines are buffered and written in batches; a
  flush makes them readable. No logger calls fsync.
- **Settings mapped where they exist, documented where they don't.** The
  latency maps to a periodic flush in spdlog (done by the adapter, like
  `spdlog::flush_every`) and to the minimal flush interval of quill's
  backend. Queue capacity has no runtime equivalent in quill. Message length
  limit exists only in soralog. Each executable prints how it mapped the
  settings.
- **Where the work goes is visible.** quill formats on its backend thread,
  so its logging call is cheap but its backend works more; spdlog-sync does
  everything in the writer. Read push together with bg CPU and throughput.

The latency is an upper bound only: below it, how soon a line reaches the
file depends on the buffers of each logger. With the default 1000 ms the
loggers keep the same promise but deliver very differently: soralog waits
for its queue to fill up to 80%, spdlog for its stdio buffer of a few
kilobytes. The `latency-10ms-*` and `latency-1ms-*` scenarios set a bound
below what any of the buffers would hold back, so delivery evens out and
the price of keeping the log that fresh is compared: the logging call,
throughput and CPU. For quill the latency is the minimal interval between
flushes rather than a bound: idle, its backend flushes sooner, under steady
load it may flush later; the delivery columns show what it does.

Versions come from the vcpkg baseline of the project and are recorded in
every result row.

To add another logger, see [Adding a logger](adding-a-logger.md).

## Running it on your hardware

Build with vcpkg in Release mode:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake \
      -DBUILD_BENCHMARK_COMPARE=ON
cmake --build build
```

Run the comparison; the log file goes into the output directory unless
`--sink-dir` points elsewhere, and its file system is part of the result:

```sh
benchmark/compare/run_compare.py run --build-dir build \
    --out docs/benchmark/results/<cpu>-<os>
```

The `standard` suite takes about ten minutes; `--suite quick` takes about
one, for a first look. `--loggers soralog,quill` limits the set. The output
directory gets:

| File | Content |
|---|---|
| `platform.json` | CPU, cores, memory, OS, file system, compiler, soralog revision; nothing identifying the machine |
| `results.csv` | one row per scenario, logger and number of writers |
| `raw.txt` | the printed tables, as the executables wrote them |
| `report.md` | tables comparing the loggers per scenario |

To compare platforms, build a report from several result directories:

```sh
benchmark/compare/run_compare.py report docs/benchmark/results/* \
    --output docs/benchmark/results/README.md
```

Single runs with any parameters:

```sh
build/benchmark/compare/compare_quill --threads 1,4 --rate 200000 \
    --delivery --sink-path /tmp/compare.log
build/benchmark/soralog_benchmark --scenario load --capacity 8192 \
    --latency 100 --rate 500000 --delivery --sink-path /tmp/load.log
```

To contribute results for a new platform, run the `standard` suite on an
idle machine and add the output directory under `docs/benchmark/results`.

## Pitfalls

- **Clock resolution.** On Apple silicon `steady_clock` ticks every 41.7 ns
  (24 MHz), so push times of tens of nanoseconds are rounded to a tick:
  "0.04 µs" means "at most one tick". The driver overhead line shows the
  floor.
- **Heterogeneous cores.** On CPUs with performance and efficiency cores the
  scheduler may put a writer or the worker on a slow core; once the threads
  outnumber the performance cores, results get noisy. Compare the spread
  (`rate_min`, `rate_max` in the CSV) before drawing conclusions.
- **Page cache, not the disk.** No logger syncs the file, so the numbers are
  for writing into the page cache. A long run may hit the writeback of
  dirty pages, which shows up as rare long stalls.
- **Delivery resolution** is the poll interval of the reader, see above.
- **Background activity** on the machine moves the tail percentiles first.
  Run on an idle machine, on AC power, and repeat.
- **Debug builds** are reported as such in the output and are not
  representative.


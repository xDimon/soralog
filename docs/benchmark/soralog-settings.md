[//]: # (
Copyright Soramitsu Co., 2021-2023
Copyright Quadrivium Co., 2023
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# soralog: how performance depends on settings and load

What the settings of the file sink and the shape of the load do to the cost
of logging and to how soon events reach the file, measured one factor at a
time around the defaults. The full tables are in the
[generated report](results/apple-m4-max-macos-27.2/soralog-settings/report.md);
this page explains them and draws the conclusions. The method and the
meaning of the columns are in the [benchmark docs](README.md).

## Measurements and assumptions — measured 2026-11-20

- Apple M4 Max (12 performance + 4 efficiency cores), 48 GB, macOS 27.2,
  APFS on the internal SSD, Homebrew clang 23, Release build, soralog 0.3.0.
- Suite `soralog-settings` of `benchmark/compare/run_compare.py`, 3 repeats
  per point; throughput and CPU as the median of the repeats, latencies as
  percentiles over all events of all repeats.
- Defaults, kept for everything not varied: queue of 2048 events, message
  limit 1024 B, write buffer 4 MB, latency 1000 ms, 64-byte messages
  (112-byte lines), logging through `Logger`.
- Every run verified the file: all events present, once, in order.
- The test machine was otherwise idle, but IDEs were open; throughput of 4 and
  more writers varies between repeats by up to ~30%, see the min–max column
  of the report before trusting a difference smaller than that.

## How the sink works, and the formula that follows

A logging call formats the message into a slot of a lock-free queue and
returns. A worker thread wakes up, writes the queued lines into the write
buffer and the buffer into the file. It wakes up when the latency timer
expires, or earlier, when the queue is 80% full. With a steady rate λ the
queue fills to 80% in `0.8 × capacity / λ`, so the worker writes every

    T = min(latency, 0.8 × capacity / λ)

and an event waits for the next write: delivery is spread over 0…T, its
median about T/2, its p99 about T. The measurements agree:

| Queue | Latency | Rate | T by the formula | Delivery p50 | Delivery p99 |
|---:|---:|---:|---:|---:|---:|
| 512 | 1000 ms | 100,000/s | 4.1 ms | 2.2 ms | 4.2 ms |
| 2048 | 1000 ms | 100,000/s | 16.4 ms | 8.4 ms | 16.8 ms |
| 8192 | 100 ms | 100,000/s | 65.5 ms | 33.6 ms | 67.1 ms |
| 2048 | 1000 ms | 10,000/s | 164 ms | 83.9 ms | 164 ms |
| 2048 | 10 ms | 100,000/s | 10 ms | 6.0 ms | 12.6 ms |
| 2048 | 1 ms | 100,000/s | 1 ms | 0.75 ms | 1.5 ms |

When the latency is the bound (the last two rows) the real period is
longer than the setting: about 1.25× at 10 ms and 1.5× at 1 ms. The cause
is not established; treat the latency as "about", not as a hard bound, when
it is small.

So under load the latency setting matters only while
`λ × latency < 0.8 × capacity`; above that, the queue size decides how fresh
the file is.

## Factor by factor

### Number of writers

| Writers | 1 | 2 | 4 | 8 | 12 |
|---|---:|---:|---:|---:|---:|
| saturation, events/s | 14.9 M | 11.5 M | 8.4 M | 4.4 M | 3.1 M |
| `push` p99.9 at saturation | 0.17 µs | 0.54 µs | 2.8 µs | 28 µs | 45 µs |
| `push` p99 at 500,000/s in total | 0.13 µs | 0.30 µs | 0.43 µs | 0.46 µs | 0.54 µs |

All writers share one queue; at saturation they contend for it and the
total throughput falls as writers are added. The same total rate split
among more writers costs little: the contention hurts only when the queue is
the bottleneck. 12 writers plus the worker exceed the 12 performance cores
of the Apple M4 Max, which adds to the drop.

### Latency

At 100,000 events/s the latency changes how fresh the file is and almost
nothing else: `push` p99.9 stays at 0.5–0.7 µs (1 writer) for 1…1000 ms,
the worker takes 1–2% of a core, saturation throughput is ~14.8 M events/s
for every latency from 1 to 1000 ms. **A short latency is cheap**: 10 ms
keeps delivery p99 at ~13 ms at no measurable cost; 1 ms gives
~1.5 ms for 2% of a core.

#### Latency 0: synchronous mode

Every logging call writes its event to the file (into the OS) before it
returns; there is no worker.

| | Asynchronous (1000 ms) | Synchronous (0) |
|---|---:|---:|
| saturation, 1 writer | 14.8 M/s | 365 k/s |
| saturation, 4 writers | 9.0 M/s | 287 k/s |
| `push` p50 at 100,000/s, 1 writer | 0.04 µs | 2.4 µs |
| `push` p99.9 at 100,000/s, 1 writer | 0.67 µs | 15 µs |
| delivery p99 at 100,000/s | 16.8 ms | 0.14 ms (the reader resolves 0.1 ms) |

Synchronous mode costs about 40× in throughput and a system call in every
logging call. Use it when an event must be in the file before the code goes
on (right before a crash-prone operation, for instance); a latency of 1 ms
gets within ~1.5 ms of that at asynchronous cost.

### Queue capacity

| Capacity | 256 | 1024 | 2048 | 8192 | 32768 |
|---|---:|---:|---:|---:|---:|
| memory (limit 1024 B) | 288 KiB | 1.1 MiB | 2.25 MiB | 9 MiB | 36 MiB |
| saturation, 1 writer | 12.0 M | 13.9 M | 15.0 M | 15.0 M | 14.5 M |
| at 1 M/s, 4 writers: worker CPU | 97% | 88% | 80% | 53% | 33% |
| at 1 M/s, 4 writers: delivery p99 | 0.3 ms | 1.0 ms | 1.8 ms | 7.1 ms | 29 ms |

Below 2048 the worker wakes too often and saturation throughput drops;
above 8192 the larger memory footprint starts to cost. At a high steady
rate the capacity trades the worker's CPU for delivery delay: a bigger queue
means fewer, larger writes, but each event waits longer. 2048–8192 is the
sweet spot for throughput.

### Message length limit

With 64-byte messages, raising the limit from 128 B to 4096 B does not
change single-writer throughput (14.8 M/s) but grows the queue from 512 KiB
to 8.25 MiB and lowers 4-writer throughput from 9.4 M to 7.0 M/s. Set the
limit to what the messages need; longer ones are cut.

### Write buffer

| Buffer | 16 KiB | 64 KiB | 256 KiB | 1 MiB | 4 MiB | 16 MiB |
|---|---:|---:|---:|---:|---:|---:|
| saturation, 1 writer | 9.2 M | 13.8 M | 14.8 M | 14.8 M | 14.8 M | 14.7 M |

A small buffer means more write calls; from 256 KiB on it no longer
matters. The default 4 MB is generous.

### Message size

| Message | 36 B | 64 B | 256 B | 1000 B |
|---|---:|---:|---:|---:|
| saturation, 1 writer | 14.1 M | 14.8 M | 12.5 M | 6.1 M (3.4–6.6 M) |
| bytes to the file | 1.2 GB/s | 1.7 GB/s | 3.8 GB/s | 6.4 GB/s |

The cost per event is nearly flat up to a few hundred bytes; with long
messages the worker writes several GB/s and throughput varies a lot between
runs (see Pitfalls in the benchmark docs).

### Rate and bursts

With 4 writers the sink keeps up with 1 M events/s at `push` p99.9 below
1 µs. At 3 M/s writers start to wait for room in the queue now and then
(response max 28 ms); at 10 M/s, past saturation, they fall behind the
schedule and the response time grows without bound (350 ms in a 3-second
run) — the closed-loop saturation is the ceiling for a steady rate.

Bursts up to 80% of the queue are absorbed; a burst larger than the queue
makes writers wait for the worker while it lasts. The remainder of a burst
below the 80% mark waits until the next burst or the latency timer: with
bursts of 16384 events every 330 ms, delivery max reached 168 ms.

The response time in the burst scenario (1.1–7.6 ms) is mostly the lateness of
the load generator itself waking up for the burst, not the logger: `push`
max is far below it. The generator sleeps through long gaps between events,
and the OS wakes it late; this affects the response columns of sparse
schedules only.

## Recommendations

| Situation | Settings |
|---|---|
| general use | defaults; consider a latency of 10–100 ms, it costs nothing measurable |
| the file must follow closely (tail -f, a crash may come) | latency 1–10 ms |
| an event must be in the file when the call returns | latency 0, expect ~300 k events/s and µs-scale calls |
| high steady rate, CPU matters more than freshness | capacity 8192, latency as needed |
| memory is tight | capacity 1024–2048, message limit to fit the messages |
| many writer threads | expect the total throughput to fall at saturation; well below it, more writers cost little |

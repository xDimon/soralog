#!/usr/bin/env python3
#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

"""Runs the logger comparison and builds reports.

  run_compare.py run --build-dir BUILD --out DIR [--suite standard|quick]
                     [--loggers soralog,spdlog-async,spdlog-sync,quill]
                     [--repeat N] [--sink-dir DIR]
      Runs every scenario of the suite for every logger, writes
      DIR/platform.json, DIR/results.csv, DIR/raw.txt and DIR/report.md.

  run_compare.py report DIR [DIR ...] [--output FILE]
      Builds a Markdown report from result directories; with several
      directories (platforms) adds a summary across them.

Only the standard library is used. The platform description holds what
affects the numbers (CPU, memory, OS, file system, compiler) and nothing
identifying the machine: no host name, user name or paths.
"""

import argparse
import csv
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

LOGGERS = {
    "soralog": ("compare_soralog", []),
    "spdlog-async": ("compare_spdlog", ["--mode", "async"]),
    "spdlog-sync": ("compare_spdlog", ["--mode", "sync"]),
    "quill": ("compare_quill", []),
}

# How each adapter maps the common settings; see the adapters' sources
SETTINGS = {
    "soralog": "Logger -> SinkToFile; event queue of `capacity` events, "
               "message limit, write buffer and latency as given; a full "
               "queue makes the producer wait.",
    "spdlog-async": "async_logger, 1 worker, queue of `capacity` messages, "
                    "overflow policy `block`; file sink as basic_file_sink_mt "
                    "(stdio buffer); latency mapped to a periodic flush.",
    "spdlog-sync": "synchronous logger: the producer formats and writes under "
                   "the sink mutex; stdio buffer; latency mapped to a periodic "
                   "flush.",
    "quill": "queue per producer of 128 KiB (compile-time), bounded blocking; "
             "the backend thread formats and writes; latency mapped to the "
             "minimal flush interval, which is not an upper bound: idle, the "
             "backend flushes sooner, under steady load it may flush later; "
             "`capacity` and message limit do not apply.",
}

# Scenario: label, arguments; every run also gets --sink-path, --csv, --label
SUITES = {
    "standard": [
        ("saturation", "closed loop, 64-byte messages",
         ["--threads", "1,2,4,8", "--ops", "1000000"]),
        ("message-256", "closed loop, 256-byte messages",
         ["--threads", "1,4", "--ops", "1000000", "--message-size", "256"]),
        ("message-1000", "closed loop, 1000-byte messages",
         ["--threads", "1,4", "--ops", "1000000", "--message-size", "1000"]),
        ("writers-500k", "500,000 events/s in total, split among writers",
         ["--threads", "1,2,4,8", "--rate", "500000", "--duration", "3",
          "--delivery"]),
        ("rate-100k", "steady 100,000 events/s",
         ["--threads", "1,4", "--rate", "100000", "--duration", "3",
          "--delivery"]),
        ("rate-1m", "steady 1,000,000 events/s",
         ["--threads", "4", "--rate", "1000000", "--duration", "3",
          "--delivery"]),
        ("burst-4096", "100,000 events/s in bursts of 4096 per writer",
         ["--threads", "2", "--rate", "100000", "--burst", "4096",
          "--duration", "3", "--delivery"]),
    ] + [
        # The same bound on the delay for all: a short latency is below what
        # the buffers of every logger would hold back, so delivery evens out
        # and the cost of keeping the log that fresh is compared
        scenario
        for latency in ("10", "1")
        for scenario in (
            (f"latency-{latency}ms-saturation",
             f"latency {latency} ms, closed loop, 64-byte messages",
             ["--threads", "1,4", "--ops", "1000000", "--latency", latency]),
            (f"latency-{latency}ms-rate-100k",
             f"latency {latency} ms, steady 100,000 events/s",
             ["--threads", "1,4", "--rate", "100000", "--duration", "3",
              "--delivery", "--latency", latency]),
            (f"latency-{latency}ms-rate-1m",
             f"latency {latency} ms, steady 1,000,000 events/s",
             ["--threads", "4", "--rate", "1000000", "--duration", "3",
              "--delivery", "--latency", latency]),
        )
    ],
    "quick": [
        ("saturation", "closed loop, 64-byte messages",
         ["--threads", "1,4", "--ops", "300000"]),
        ("rate-100k", "steady 100,000 events/s",
         ["--threads", "2", "--rate", "100000", "--duration", "1",
          "--delivery"]),
    ],
}

# Dependence of soralog on its settings and on the load: one factor at a time
# around the defaults (queue 2048 events, message limit 1024 B, write buffer
# 4 MB, latency 1000 ms, 64-byte messages), and a grid where two factors act
# together. Label -> (CSV columns of the factor, description); every label may
# have several runs, one per value.
CLOSED = ["--ops", "1000000"]
OPEN = ["--duration", "3", "--delivery"]
FACTORS = {
    "capacity-closed": (["capacity"], "queue capacity, events; closed loop"),
    "capacity-1m": (["capacity"],
                    "queue capacity, events; 1,000,000 events/s"),
    "message-length": (["message_length"],
                       "message length limit, bytes; 64-byte messages, "
                       "closed loop"),
    "buffer-size": (["buffer_size"], "write buffer, bytes; closed loop"),
    "latency-closed": (["latency_ms"], "latency, ms (0: synchronous); "
                                       "closed loop"),
    "latency-100k": (["latency_ms"], "latency, ms (0: synchronous); "
                                     "100,000 events/s"),
    "message-size": (["message_size"], "message size, bytes; closed loop"),
    "writers-closed": (["threads"], "writer threads; closed loop"),
    "writers-500k": (["threads"], "writer threads sharing 500,000 events/s"),
    "rate": (["rate"], "events per second in total, 4 writers"),
    "burst": (["burst"], "events per burst of each of 2 writers, "
                         "100,000 events/s in total"),
    "capacity-x-latency": (["capacity", "latency_ms"],
                           "queue capacity × latency, 1 writer, "
                           "100,000 events/s"),
}
SUITES["soralog-settings"] = (
    [("capacity-closed", "", ["--threads", "1,4", "--capacity", v] + CLOSED)
     for v in ("256", "1024", "2048", "8192", "32768")]
    + [("capacity-1m", "", ["--threads", "4", "--rate", "1000000",
                            "--capacity", v] + OPEN)
       for v in ("256", "1024", "2048", "8192", "32768")]
    + [("message-length", "", ["--threads", "1,4", "--message-length", v]
        + CLOSED) for v in ("128", "256", "1024", "4096")]
    + [("buffer-size", "", ["--threads", "1,4", "--buffer-size", v] + CLOSED)
       for v in ("16384", "65536", "262144", "1048576", "4194304",
                 "16777216")]
    + [("latency-closed", "", ["--threads", "1,4", "--latency", v, "--ops",
                               "200000" if v == "0" else "1000000"])
       for v in ("0", "1", "10", "100", "1000")]
    + [("latency-100k", "", ["--threads", "1,4", "--rate", "100000",
                             "--latency", v] + OPEN)
       for v in ("0", "1", "10", "100", "1000")]
    + [("message-size", "", ["--threads", "1,4", "--message-size", v]
        + CLOSED) for v in ("36", "64", "256", "1000")]
    + [("writers-closed", "", ["--threads", "1,2,4,8,12"] + CLOSED),
       ("writers-500k", "", ["--threads", "1,2,4,8,12", "--rate", "500000"]
        + OPEN)]
    + [("rate", "", ["--threads", "4", "--rate", v] + OPEN)
       for v in ("10000", "100000", "1000000", "3000000", "10000000")]
    + [("burst", "", ["--threads", "2", "--rate", "100000", "--burst", v]
        + OPEN) for v in ("256", "1024", "2048", "4096", "16384")]
    + [("capacity-x-latency", "", ["--threads", "1", "--rate", "100000",
                                   "--capacity", c, "--latency", l] + OPEN)
       for c in ("512", "2048", "8192") for l in ("10", "100", "1000")]
)

DEFAULT_REPEAT = {"standard": 3, "quick": 1, "soralog-settings": 3}


def command(args):
    try:
        return subprocess.run(args, capture_output=True, text=True,
                              check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def sysctl(name):
    return command(["sysctl", "-n", name])


def file_system(path):
    """Type of the file system holding the path, and whether it is an SSD."""
    if sys.platform == "darwin":
        mount = command(["df", str(path)]).splitlines()[-1].split()[-1]
        info = command(["diskutil", "info", mount])
        kind = re.search(r"File System Personality:\s*(.+)", info)
        ssd = re.search(r"Solid State:\s*(.+)", info)
        return {"type": kind.group(1) if kind else "",
                "solid_state": ssd.group(1) if ssd else ""}
    kind = command(["stat", "-f", "-c", "%T", str(path)])
    device = command(["findmnt", "-no", "SOURCE", "-T", str(path)])
    rotational = ""
    name = os.path.basename(device)
    for candidate in (name, re.sub(r"p?\d+$", "", name)):
        flag = Path(f"/sys/block/{candidate}/queue/rotational")
        if candidate and flag.exists():
            rotational = flag.read_text().strip()
            break
    return {"type": kind,
            "solid_state": {"0": "Yes", "1": "No"}.get(rotational, "")}


def describe_platform(build_dir, sink_dir):
    info = {"os": f"{platform.system()} {platform.release()}",
            "arch": platform.machine()}
    if sys.platform == "darwin":
        info["os"] = f"macOS {platform.mac_ver()[0]}"
        info["cpu"] = sysctl("machdep.cpu.brand_string")
        info["cores"] = sysctl("hw.physicalcpu")
        p_cores = sysctl("hw.perflevel0.physicalcpu")
        e_cores = sysctl("hw.perflevel1.physicalcpu")
        if p_cores and e_cores:
            info["cores"] += f" ({p_cores} performance + {e_cores} efficiency)"
        memory = sysctl("hw.memsize")
        info["memory_gb"] = round(int(memory) / 2**30) if memory else ""
    else:
        cpuinfo = Path("/proc/cpuinfo")
        text = cpuinfo.read_text() if cpuinfo.exists() else ""
        model = re.search(r"model name\s*:\s*(.+)", text)
        info["cpu"] = model.group(1).strip() if model else platform.processor()
        info["cores"] = command(["nproc"])
        meminfo = Path("/proc/meminfo")
        total = re.search(r"MemTotal:\s*(\d+)",
                          meminfo.read_text() if meminfo.exists() else "")
        info["memory_gb"] = round(int(total.group(1)) / 2**20) if total else ""
        release = Path("/etc/os-release")
        name = re.search(r'PRETTY_NAME="(.+)"',
                         release.read_text() if release.exists() else "")
        if name:
            info["os"] = f"{name.group(1)} (kernel {platform.release()})"
    info["file_system"] = file_system(sink_dir)

    cache = Path(build_dir) / "CMakeCache.txt"
    text = cache.read_text() if cache.exists() else ""
    compiler = re.search(r"^CMAKE_CXX_COMPILER:\w+=(.+)$", text, re.M)
    build_type = re.search(r"^CMAKE_BUILD_TYPE:\w+=(.*)$", text, re.M)
    info["compiler"] = (command([compiler.group(1), "--version"])
                        .splitlines() or [""])[0] if compiler else ""
    info["build_type"] = build_type.group(1) if build_type else ""

    source = Path(__file__).resolve().parents[2]
    revision = command(["git", "-C", str(source), "rev-parse", "--short",
                        "HEAD"])
    dirty = command(["git", "-C", str(source), "status", "--porcelain",
                     "--untracked-files=no"])
    info["soralog_revision"] = revision + ("+changes" if dirty else "")
    return info


def run(args):
    build = Path(args.build_dir).resolve()
    binaries = build / "benchmark" / "compare"
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    sink_dir = Path(args.sink_dir).resolve() if args.sink_dir else out / "tmp"
    sink_dir.mkdir(parents=True, exist_ok=True)
    sink = sink_dir / "compare.log"
    loggers = args.loggers.split(",")
    if args.suite == "soralog-settings":
        loggers = ["soralog"]
    for name in loggers:
        if name not in LOGGERS:
            sys.exit(f"unknown logger {name}; known: {', '.join(LOGGERS)}")
        if not (binaries / LOGGERS[name][0]).exists():
            sys.exit(f"{binaries / LOGGERS[name][0]} not found: build with "
                     "-DBUILD_BENCHMARK_COMPARE=ON")
    repeat = args.repeat or DEFAULT_REPEAT[args.suite]

    info = describe_platform(build, sink_dir)
    info["suite"] = args.suite
    info["repeat"] = repeat
    (out / "platform.json").write_text(json.dumps(info, indent=2) + "\n")

    results = out / "results.csv"
    results.unlink(missing_ok=True)
    failed = False
    with open(out / "raw.txt", "w") as raw:
        for label, description, scenario in SUITES[args.suite]:
            for name in loggers:
                binary, mode = LOGGERS[name]
                cmd = ([str(binaries / binary)] + mode + scenario
                       + ["--repeat", str(repeat), "--sink-path", str(sink),
                          "--csv", str(results), "--label", label])
                print(f"-- {label}: {name}", flush=True)
                done = subprocess.run(cmd, capture_output=True, text=True)
                raw.write(done.stdout + done.stderr + "\n")
                raw.flush()
                if done.returncode != 0:
                    failed = True
                    print(done.stderr.strip(), file=sys.stderr)
    sink.unlink(missing_ok=True)
    if not args.sink_dir:
        shutil.rmtree(sink_dir, ignore_errors=True)

    (out / "report.md").write_text(build_report([out]))
    print(f"results: {out}")
    return 1 if failed else 0


# ------------------------------------------------------------------ report

def us(ns):
    if ns in ("", None):
        return "–"
    value = float(ns) / 1e3
    if value < 10:
        return f"{value:.2f}"
    if value < 1000:
        return f"{value:.1f}"
    return f"{value:,.0f}"


def rate(value):
    value = float(value)
    if value >= 1e6:
        return f"{value / 1e6:.2f} M"
    if value >= 1e3:
        return f"{value / 1e3:.1f} k"
    return f"{value:.0f}"


def load(directory):
    directory = Path(directory)
    info = json.loads((directory / "platform.json").read_text())
    with open(directory / "results.csv") as f:
        rows = list(csv.DictReader(f))
    return info, rows


def platform_title(info):
    return f"{info.get('cpu', '?')}, {info.get('os', '?')}"


def platform_lines(info):
    fs = info.get("file_system", {})
    disk = fs.get("type", "")
    if fs.get("solid_state") == "Yes":
        disk += ", SSD"
    elif fs.get("solid_state") == "No":
        disk += ", rotational disk"
    return [
        f"- CPU: {info.get('cpu', '')}, {info.get('arch', '')}, "
        f"cores: {info.get('cores', '')}",
        f"- Memory: {info.get('memory_gb', '')} GB",
        f"- OS: {info.get('os', '')}",
        f"- File system of the log file: {disk}",
        f"- Compiler: {info.get('compiler', '')}, "
        f"build type {info.get('build_type', '')}",
        f"- soralog revision: {info.get('soralog_revision', '')}, "
        f"suite `{info.get('suite', '')}`, {info.get('repeat', '')} repeats",
    ]


def highlight_best(table):
    """Rows of (group, cells); within a group the best value of each column
    is set in bold, ties included, when the group has more than one row."""
    lines = []
    for group in dict.fromkeys(g for g, _ in table):
        rows = [cells for g, cells in table if g == group]
        best = {}
        for i, (_, value, better) in enumerate(rows[0]):
            values = [row[i][1] for row in rows if row[i][1] is not None]
            if better and len(values) > 1:
                best[i] = max(values) if better == "max" else min(values)
        for cells in rows:
            texts = []
            for i, (text, value, _) in enumerate(cells):
                winner = i in best and value is not None and (
                    text == next(t for t, v, _ in
                                 (row[i] for row in rows) if v == best[i]))
                texts.append(f"**{text}**" if winner else text)
            lines.append("| " + " | ".join(texts) + " |")
    return lines


def scenario_tables(rows):
    lines = []
    descriptions = {label: text for suite in SUITES.values()
                    for label, text, _ in suite}
    labels = list(dict.fromkeys(row["label"] for row in rows))
    for label in labels:
        group = [r for r in rows if r["label"] == label]
        first = group[0]
        open_loop = float(first["rate"]) > 0
        delivery = any(r["dlv_p50_ns"] for r in group)
        lines += [f"### {label}", "",
                  f"{descriptions.get(label, '')}; message "
                  f"{first['message_size']} B, burst {first['burst']}, "
                  f"latency {first['latency_ms']} ms, "
                  f"{first['events']} measured events per run.", ""]
        header = ["Writers", "Logger", "Events/s"]
        if not open_loop:
            header.append("min–max")
        header += ["push p50", "push p99", "push p99.9", "push max"]
        if open_loop:
            header += ["resp p99.9", "resp max"]
        if delivery:
            header += ["dlv p50", "dlv p99", "dlv max"]
        header += ["bg CPU", "Verified"]
        lines.append("| " + " | ".join(header) + " |")
        lines.append("|" + "|".join(["---:"] + [":---"] + ["---:"] *
                                    (len(header) - 2)) + "|")
        group.sort(key=lambda r: (int(r["threads"]),
                                  list(LOGGERS).index(r["logger"])
                                  if r["logger"] in LOGGERS else 99))
        table = []
        for r in group:
            # (text, value to compare, better is "max" or "min" or None)
            cells = [(r["threads"], None, None),
                     (f"{r['logger']} {r['version']}", None, None),
                     # with a set rate every logger keeps it: nothing to pick
                     (rate(r["rate_med"]), float(r["rate_med"]),
                      None if open_loop else "max")]
            if not open_loop:
                cells.append((f"{rate(r['rate_min'])}–{rate(r['rate_max'])}",
                              None, None))
            keys = ["push_p50_ns", "push_p99_ns", "push_p999_ns",
                    "push_max_ns"]
            if open_loop:
                keys += ["resp_p999_ns", "resp_max_ns"]
            if delivery:
                keys += ["dlv_p50_ns", "dlv_p99_ns", "dlv_max_ns"]
            cells += [(us(r[k]), float(r[k]) if r[k] else None, "min")
                      for k in keys]
            cells += [(f"{float(r['bg_cpu_pct']):.0f}%", None, None),
                      ("yes" if r["ok"] == "1" else "**FAILED**", None, None)]
            table.append((r["threads"], cells))
        lines += highlight_best(table)
        lines.append("")
    return lines


def pick(rows, label, logger, threads=None):
    found = [r for r in rows if r["label"] == label and r["logger"] == logger]
    if threads is not None:
        found = [r for r in found if int(r["threads"]) == threads]
    return found


def summary(platforms):
    """One line per platform and logger: key numbers of key scenarios."""
    lines = ["## Summary", "",
             "Saturation throughput with 1 and the most writers; at a steady "
             "100,000 events/s with 1 writer: push p99.9 and delivery p99.", "",
             "| Platform | Logger | Events/s, 1 writer | Events/s, most "
             "writers | push p99.9 at 100k/s | dlv p99 at 100k/s |",
             "|:---|:---|---:|---:|---:|---:|"]
    table = []
    for info, rows in platforms:
        title = platform_title(info)
        loggers = list(dict.fromkeys(r["logger"] for r in rows))
        for logger in loggers:
            sat = pick(rows, "saturation", logger)
            one = [r for r in sat if r["threads"] == "1"]
            most = max(sat, key=lambda r: int(r["threads"])) if sat else None
            steady = (pick(rows, "rate-100k", logger, 1)
                      or pick(rows, "rate-100k", logger))
            value = lambda row, key: float(row[key]) if row and row[key] \
                else None
            cells = [
                (title, None, None), (logger, None, None),
                (rate(one[0]["rate_med"]) if one else "–",
                 value(one[0], "rate_med") if one else None, "max"),
                (f"{rate(most['rate_med'])} ({most['threads']})" if most
                 else "–", value(most, "rate_med"), "max"),
                (us(steady[0]["push_p999_ns"]) if steady else "–",
                 value(steady[0], "push_p999_ns") if steady else None, "min"),
                (us(steady[0]["dlv_p99_ns"]) if steady else "–",
                 value(steady[0], "dlv_p99_ns") if steady else None, "min"),
            ]
            table.append((title, cells))
    lines += highlight_best(table)
    lines.append("")
    return lines


def queue_memory(row):
    """Memory of the soralog event queue: a slot holds 112 bytes of event
    and the message limit, rounded up to 128 bytes."""
    slot = (112 + int(row["message_length"]) + 127) // 128 * 128
    size = int(row["capacity"]) * slot
    return f"{size / 2**20:.2f} MiB" if size >= 2**20 else f"{size // 1024} KiB"


def settings_tables(rows):
    lines = []
    for label, (keys, description) in FACTORS.items():
        group = [r for r in rows if r["label"] == label]
        if not group:
            continue
        open_loop = float(group[0]["rate"]) > 0
        delivery = any(r["dlv_p50_ns"] for r in group)
        memory = any(k in ("capacity", "message_length") for k in keys)
        factor = " × ".join(keys)
        lines += [f"### {label}", "", f"Varied: {description}.", ""]
        writers = keys != ["threads"]
        header = [factor if writers else "Writers"]
        header += ["Writers"] if writers else []
        header.append("Events/s")
        if not open_loop:
            header.append("min–max")
        header += ["push p50", "push p99", "push p99.9", "push max"]
        if open_loop:
            header += ["resp p99.9", "resp max"]
        if delivery:
            header += ["dlv p50", "dlv p99", "dlv max"]
        header.append("bg CPU")
        if memory:
            header.append("queue memory")
        header.append("Verified")
        lines.append("| " + " | ".join(header) + " |")
        lines.append("|" + "|".join(["---:"] * len(header)) + "|")
        number = lambda v: float(v) if v else 0
        group.sort(key=lambda r: [number(r[k]) for k in keys]
                   + [int(r["threads"])])
        for r in group:
            value = " × ".join(
                ("default" if not r[k] else
                 rate(r[k]) if k == "rate" else r[k]) for k in keys)
            cells = [value] + ([r["threads"]] if writers else [])
            cells.append(rate(r["rate_med"]))
            if not open_loop:
                cells.append(f"{rate(r['rate_min'])}–{rate(r['rate_max'])}")
            cells += [us(r[k]) for k in ("push_p50_ns", "push_p99_ns",
                                         "push_p999_ns", "push_max_ns")]
            if open_loop:
                cells += [us(r["resp_p999_ns"]), us(r["resp_max_ns"])]
            if delivery:
                cells += [us(r["dlv_p50_ns"]), us(r["dlv_p99_ns"]),
                          us(r["dlv_max_ns"])]
            cells.append(f"{float(r['bg_cpu_pct']):.0f}%")
            if memory:
                cells.append(queue_memory(r))
            cells.append("yes" if r["ok"] == "1" else "**FAILED**")
            lines.append("| " + " | ".join(cells) + " |")
        lines.append("")
    return lines


def settings_report(platforms):
    lines = ["# soralog: dependence on settings and load", "",
             "Generated by `benchmark/compare/run_compare.py` (suite "
             "`soralog-settings`). One factor at a time around the defaults: "
             "queue of 2048 events, message limit 1024 B, write buffer 4 MB, "
             "latency 1000 ms, 64-byte messages. Times in µs, columns as in "
             "the comparison report; see `docs/benchmark/README.md`.", ""]
    for info, rows in platforms:
        lines += [f"## {platform_title(info)}", ""] + platform_lines(info)
        lines.append("")
        lines += settings_tables(rows)
    return "\n".join(lines)


def build_report(directories):
    platforms = [load(d) for d in directories]
    if all(info.get("suite") == "soralog-settings" for info, _ in platforms):
        return settings_report(platforms)
    lines = ["# Logger comparison", "",
             "Generated by `benchmark/compare/run_compare.py`. Times in µs. "
             "push: duration of the logging call; resp: from the scheduled "
             "moment of the event to the end of the call (open loop only); "
             "dlv: from the call to the line being readable from the file; "
             "bg CPU: the logger's own threads, % of one core; min–max: "
             "throughput range over the repeats. The best value of a column among "
             "the loggers with the same number of writers is in bold. See "
             "`docs/benchmark/README.md` for the method.", ""]
    if len(platforms) > 1:
        lines += summary(platforms)
    lines += ["## Logger settings", ""]
    lines += [f"- **{name}**: {text}" for name, text in SETTINGS.items()]
    lines.append("")
    for info, rows in platforms:
        lines += [f"## {platform_title(info)}", ""] + platform_lines(info)
        lines.append("")
        if len(platforms) == 1:
            lines += summary(platforms)
        lines += scenario_tables(rows)
    return "\n".join(lines)


def report(args):
    text = build_report(args.dirs)
    if args.output:
        Path(args.output).write_text(text)
    else:
        print(text)
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("run", help="run the comparison")
    p.add_argument("--build-dir", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--suite", choices=list(SUITES), default="standard")
    p.add_argument("--loggers", default=",".join(LOGGERS))
    p.add_argument("--repeat", type=int)
    p.add_argument("--sink-dir",
                   help="directory for the log file (default: inside --out); "
                        "its file system is part of the measurement")
    p.set_defaults(func=run)
    p = sub.add_parser("report", help="build a report from result dirs")
    p.add_argument("dirs", nargs="+")
    p.add_argument("--output")
    p.set_defaults(func=report)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

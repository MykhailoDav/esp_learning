#!/usr/bin/env python3
"""
Parse CSV output produced by ESP32-S3 firmware and build summary plots.

Usage:
    python plot_results.py results.csv

The input file must contain the section between the lines
"===BEGIN===" and "===END===" produced by the firmware. Lines outside
that range are ignored.
"""

import sys
import os
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


TYPE_COLORS = {
    "int": "tab:blue",
    "float": "tab:orange",
    "double": "tab:red",
    "char": "tab:green",
}
ALGORITHMS = ["quicksort", "bintree"]
FREQS = [240, 160, 80, 40]
SIZES = [50, 100, 500, 1000]
TYPES = ["int", "float", "double", "char"]


def load_csv(path: str) -> pd.DataFrame:
    raw = Path(path).read_text(encoding="utf-8", errors="ignore").splitlines()
    in_block = False
    data_lines = []
    for line in raw:
        s = line.strip()
        if s == "===BEGIN===":
            in_block = True
            continue
        if s == "===END===":
            in_block = False
            continue
        if not in_block:
            continue
        if not s or s.startswith("#"):
            continue
        data_lines.append(s)

    if not data_lines:
        raise SystemExit(
            "No data rows found between ===BEGIN=== and ===END=== markers"
        )

    header = data_lines[0]
    rows = data_lines[1:]
    csv_text = "\n".join([header] + rows)
    from io import StringIO
    df = pd.read_csv(StringIO(csv_text))
    df["time_ms"] = df["time_us"] / 1000.0
    df["memory_kb"] = df["memory_bytes"] / 1024.0
    return df


def plot_time_vs_size(df: pd.DataFrame, out_dir: Path):
    for algo in ALGORITHMS:
        fig, axes = plt.subplots(2, 2, figsize=(11, 8), sharey=False)
        for ax, freq in zip(axes.flat, FREQS):
            sub = df[(df.algorithm == algo) & (df.freq_mhz == freq)]
            for t in TYPES:
                s = sub[sub.type == t].sort_values("size")
                if s.empty:
                    continue
                ax.plot(s["size"], s["time_ms"], marker="o",
                        label=t, color=TYPE_COLORS[t])
            ax.set_title(f"{algo} @ {freq} MHz")
            ax.set_xlabel("Array size")
            ax.set_ylabel("Time (ms)")
            ax.grid(True, alpha=0.3)
            ax.legend()
        fig.suptitle(f"Час виконання {algo} vs розмір масиву")
        fig.tight_layout()
        path = out_dir / f"time_vs_size_{algo}.png"
        fig.savefig(path, dpi=130)
        plt.close(fig)
        print(f"saved {path}")


def plot_time_vs_freq(df: pd.DataFrame, out_dir: Path):
    for algo in ALGORITHMS:
        fig, axes = plt.subplots(2, 2, figsize=(11, 8))
        for ax, n in zip(axes.flat, SIZES):
            sub = df[(df.algorithm == algo) & (df["size"] == n)]
            for t in TYPES:
                s = sub[sub.type == t].sort_values("freq_mhz")
                if s.empty:
                    continue
                ax.plot(s["freq_mhz"], s["time_ms"], marker="s",
                        label=t, color=TYPE_COLORS[t])
            ax.set_title(f"{algo}, N = {n}")
            ax.set_xlabel("CPU frequency (MHz)")
            ax.set_ylabel("Time (ms)")
            ax.invert_xaxis()
            ax.grid(True, alpha=0.3)
            ax.legend()
        fig.suptitle(f"Час виконання {algo} vs частота CPU")
        fig.tight_layout()
        path = out_dir / f"time_vs_freq_{algo}.png"
        fig.savefig(path, dpi=130)
        plt.close(fig)
        print(f"saved {path}")


def plot_memory(df: pd.DataFrame, out_dir: Path):
    sub = df[df.freq_mhz == 240]
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    for ax, algo in zip(axes, ALGORITHMS):
        d = sub[sub.algorithm == algo]
        for t in TYPES:
            s = d[d.type == t].sort_values("size")
            if s.empty:
                continue
            ax.plot(s["size"], s["memory_kb"], marker="o",
                    label=t, color=TYPE_COLORS[t])
        ax.set_title(f"Пам'ять — {algo}")
        ax.set_xlabel("Array size")
        ax.set_ylabel("Memory (KB)")
        ax.grid(True, alpha=0.3)
        ax.legend()
    fig.suptitle("Витрати пам'яті vs розмір масиву")
    fig.tight_layout()
    path = out_dir / "memory_vs_size.png"
    fig.savefig(path, dpi=130)
    plt.close(fig)
    print(f"saved {path}")


def plot_algo_compare(df: pd.DataFrame, out_dir: Path):
    sub = df[df.freq_mhz == 240]
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    for ax, t in zip(axes, ["int", "double"]):
        d = sub[sub.type == t]
        for algo in ALGORITHMS:
            s = d[d.algorithm == algo].sort_values("size")
            ax.plot(s["size"], s["time_ms"], marker="o", label=algo)
        ax.set_title(f"quicksort vs bintree — {t} @ 240 MHz")
        ax.set_xlabel("Array size")
        ax.set_ylabel("Time (ms)")
        ax.grid(True, alpha=0.3)
        ax.legend()
    fig.tight_layout()
    path = out_dir / "algo_compare.png"
    fig.savefig(path, dpi=130)
    plt.close(fig)
    print(f"saved {path}")


def write_summary_table(df: pd.DataFrame, out_dir: Path):
    pivot_t = df.pivot_table(index=["algorithm", "type", "size"],
                             columns="freq_mhz",
                             values="time_us")
    pivot_m = df.pivot_table(index=["algorithm", "type", "size"],
                             columns="freq_mhz",
                             values="memory_bytes")
    out_t = out_dir / "summary_time_us.csv"
    out_m = out_dir / "summary_memory_bytes.csv"
    pivot_t.to_csv(out_t)
    pivot_m.to_csv(out_m)
    print(f"saved {out_t}")
    print(f"saved {out_m}")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    src = sys.argv[1]
    out_dir = Path("plots")
    out_dir.mkdir(exist_ok=True)
    df = load_csv(src)
    print(f"loaded {len(df)} rows from {src}")
    plot_time_vs_size(df, out_dir)
    plot_time_vs_freq(df, out_dir)
    plot_memory(df, out_dir)
    plot_algo_compare(df, out_dir)
    write_summary_table(df, out_dir)
    print(f"done. output in: {out_dir.resolve()}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import os
import re
import sys
import glob
import statistics
from datetime import datetime

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

DIRPATH = "./vmem_files"

# separator between hms is ignored
NAME_RE = re.compile(
	r"^map_(?P<pid>\d+)"
	r"_(?P<date>\d{4}-\d{2}-\d{2})"
	r"_(?P<h>\d{1,2})\D+(?P<m>\d{1,2})\D+(?P<s>\d{1,2})"
	r"_(?P<mode>p[12])\.(?:txt|tx)$"
)

def parse_pmap(path):
	heap_kb = heap_rss = 0
	total_kb = total_rss = 0

	with open(path, "r", errors="replace") as f:
		for line in f:
			parts = line.split()
			if not parts:
				continue

			# [heap] line: ... <kb> <rss> <dirty> ... [heap]
			if "[heap]" in parts and len(parts) >= 6:
				try:
					heap_kb  = int(parts[1])
					heap_rss = int(parts[2])
				except ValueError:
					pass

			# total line: total kB <vsz> <rss> <dirty>
			if parts[0] == "total" and len(parts) >= 5 and parts[1] == "kB":
				try:
					total_kb  = int(parts[2])
					total_rss = int(parts[3])
				except ValueError:
					pass

	return {
		"heap_kb":   heap_kb,
		"heap_rss":  heap_rss,
		"total_kb":  total_kb,
		"total_rss": total_rss,
	}


def collect(dirpath, n_last):
	by_mode = {"p1": [], "p2": []}
	skipped = 0

	for path in glob.glob(os.path.join(dirpath, "map_*.txt")) + \
				glob.glob(os.path.join(dirpath, "map_*.tx")):
		m = NAME_RE.match(os.path.basename(path))
		if not m:
			skipped += 1
			continue

		try:
			ts = datetime(
				int(m.group("date")[0:4]),
				int(m.group("date")[5:7]),
				int(m.group("date")[8:10]),
				int(m.group("h")),
				int(m.group("m")),
				int(m.group("s")),
			)
		except ValueError:
			skipped += 1
			continue

		metrics = parse_pmap(path)
		metrics["pid"]  = int(m.group("pid"))
		metrics["path"] = path
		by_mode[m.group("mode")].append((ts, metrics))

	if skipped:
		print(f"warning: {skipped} file(s) did not match the name pattern", file=sys.stderr)

	for mode in by_mode:
		by_mode[mode].sort(key=lambda x: x[0])
		if n_last and len(by_mode[mode]) > n_last:
			by_mode[mode] = by_mode[mode][-n_last:]

	return by_mode


def stats(values):
	if not values:
		return dict(start=0, end=0, min=0, max=0, mean=0.0, std=0.0)
	return dict(
		start=values[0],
		end=values[-1],
		min=min(values),
		max=max(values),
		mean=statistics.mean(values),
		std=statistics.pstdev(values) if len(values) > 1 else 0.0,
	)


def plot_mode(mode, series, outdir):
	if not series:
		print(f"{mode}: no data, skipping")
		return

	times     = [t for t, _ in series]
	heap_kb   = [m["heap_kb"]   for _, m in series]
	heap_rss  = [m["heap_rss"]  for _, m in series]
	total_rss = [m["total_rss"] for _, m in series]
	pids      = [m["pid"]       for _, m in series]

	fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)

	ax1.plot(times, heap_kb,  marker="o", label="heap Kbytes")
	ax1.plot(times, heap_rss, marker="s", label="heap RSS")
	ax1.set_ylabel("KiB")
	ax1.set_title(f"{mode}: heap over time")
	ax1.legend()
	ax1.grid(True, alpha=0.3)

	ax2.plot(times, total_rss, marker="o", color="tab:red", label="total RSS")
	ax2.set_ylabel("KiB")
	ax2.set_xlabel("time")
	ax2.set_title(f"{mode}: total RSS over time")
	ax2.legend()
	ax2.grid(True, alpha=0.3)

	fig.autofmt_xdate()
	fig.tight_layout()

	png = os.path.join(outdir, f"plot_{mode}_heap_rss.png")
	fig.savefig(png, dpi=120)
	plt.close(fig)
	print(f"{mode}: wrote {png}  ({len(series)} points, pids {min(pids)}..{max(pids)})")

	txt = os.path.join(outdir, f"summary_{mode}.txt")
	with open(txt, "w") as f:
		f.write(f"mode = {mode}\n")
		f.write(f"files = {len(series)}\n")
		f.write(f"pids  = {sorted(set(pids))}\n")
		f.write(f"first = {times[0]}\n")
		f.write(f"last  = {times[-1]}\n")
		for name, s in (("heap_kb",   stats(heap_kb)),
						("heap_rss",  stats(heap_rss)),
						("total_rss", stats(total_rss))):
			f.write(f"\n{name}:\n")
			for k, v in s.items():
				f.write(f"  {k:5s} = {v:.1f}\n")
	print(f"{mode}: wrote {txt}")


def main():
	n_last = int(sys.argv[1]) if len(sys.argv) > 1 else 0

	if not os.path.isdir(DIRPATH):
		print(f"error: {DIRPATH} is not a directory", file=sys.stderr)
		sys.exit(1)

	by_mode = collect(DIRPATH, n_last)
	for mode in ("p1", "p2"):
		plot_mode(mode, by_mode.get(mode, []), DIRPATH)


if __name__ == "__main__":
	main()

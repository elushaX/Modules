import json
import sys
import re
import matplotlib.pyplot as plt

def load_bench(path):
    with open(path, "r") as f:
        return json.load(f)["benchmarks"]

def parse_arg_from_name(name):
    # Example: "BM_VectorPushBack/1000" -> 1000
    m = re.search(r"/(\d+)$", name)
    return int(m.group(1)) if m else None

def group_by_prefix(benchmarks):
    groups = {}
    for b in benchmarks:
        name = b["name"]
        prefix = name.split("/")[0]       # BM_VectorPushBack
        arg = parse_arg_from_name(name)   # number after slash
        if arg is None:
            continue
        if prefix not in groups:
            groups[prefix] = {"x": [], "y": []}
        groups[prefix]["x"].append(arg)
        groups[prefix]["y"].append(b["real_time"])
    return groups

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 plot_bench.py results.json")
        return

    benchmarks = load_bench(sys.argv[1])
    groups = group_by_prefix(benchmarks)

    plt.figure(figsize=(10, 6))

    for prefix, data in groups.items():
        xs = data["x"]
        ys = data["y"]
        plt.plot(xs, ys, marker="o", label=prefix)

    plt.xlabel("Input size (Arg)")
    plt.ylabel("Real time (ns)")
    plt.title("Google Benchmark Results")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()

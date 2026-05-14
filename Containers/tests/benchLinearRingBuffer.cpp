#include <benchmark/benchmark.h>
#include <vector>
#include <map>

// -------------------------------------------------------------
// Benchmark: push_back into a vector
// -------------------------------------------------------------
static void BM_VectorPushBack(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<int> v;
		v.reserve(state.range(0));
		for (int i = 0; i < state.range(0); i++) {
			v.push_back(i);
		}
		benchmark::DoNotOptimize(v);
	}
}

BENCHMARK(BM_VectorPushBack)->Arg(1000)->Arg(100000);

// -------------------------------------------------------------
// Benchmark: inserting into std::map
// -------------------------------------------------------------
static void BM_MapInsert(benchmark::State& state) {
	for (auto _ : state) {
		std::map<int, int> m;
		for (int i = 0; i < state.range(0); i++) {
			m.emplace(i, i);
		}
		benchmark::DoNotOptimize(m);
	}
}
BENCHMARK(BM_MapInsert)->Arg(1000)->Arg(100000);

// -------------------------------------------------------------
// Benchmark: lookup in std::map
// -------------------------------------------------------------
static void BM_MapLookup(benchmark::State& state) {
	std::map<int, int> m;
	for (int i = 0; i < state.range(0); i++)
		m.emplace(i, i);

	for (auto _ : state) {
		auto it = m.find(state.range(0) / 2);
		benchmark::DoNotOptimize(it);
	}
}
BENCHMARK(BM_MapLookup)->Arg(1000)->Arg(100000);

BENCHMARK_MAIN();

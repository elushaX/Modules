#pragma once

#include "BenchReport.hpp"
#include "BenchScenario.hpp"

#include <map>
#include <string>
#include <vector>

namespace tp::sketch2d {
	class Sketch2DApplication;
}

namespace tp::sketch2d::bench {

	struct RunOptions {
		int repeats = 1;
		bool pacedByCompositor = true;
	};

	class BenchRunner {
	public:
		BenchRunner(Sketch2DApplication& application, RunOptions options);
		~BenchRunner();

		BenchRunner(const BenchRunner&) = delete;
		BenchRunner& operator=(const BenchRunner&) = delete;

		[[nodiscard]] ScenarioResult run(const Scenario& scenario, const InputRecording& input);

	private:
		void apply(const Scenario& scenario);
		void resetDocument();
		void prepaint();
		void replay(const InputRecording& input, std::vector<double>& latencies, size_t& frames);
		void drain(size_t& frames);

		Sketch2DApplication& mApplication;
		RunOptions mOptions;
	};
}

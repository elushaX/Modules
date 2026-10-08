#pragma once

#include "Brush.hpp"
#include "Session.hpp"
#include "InputRecording.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tp::sketch2d::bench {

	struct Scenario {
		std::string name;
		std::string group;
		std::string input;
		BrushType brush = BrushType::Round;
		halnf size = 20;
		halnf zoom = 1;
		halnf stabilizer = 0;
		bool eraserMode = false;
		bool prepaint = false;
		std::function<void(Brush&)> tune;
		std::shared_ptr<const SessionSnapshot> session;

		[[nodiscard]] std::string describe() const;
	};

	struct BenchInputs {
		std::string name;
		InputRecording recording;
	};

	[[nodiscard]] std::vector<Scenario> brushAndEraserScenarios(const std::vector<std::string>& inputs);
}

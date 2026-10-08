#include "BenchScenario.hpp"

#include <cctype>
#include <cstdio>

namespace tp::sketch2d::bench {

	namespace {
		std::string number(halnf value) {
			char text[32];
			std::snprintf(text, sizeof(text), "%g", value);
			return text;
		}

		std::string presetLabel(BrushType type) {
			const BrushPreset* preset = findBrushPreset(type);
			return preset ? preset->name : "brush";
		}

		std::string slug(std::string text) {
			for (char& character : text) {
				if (character == ' ') character = '-';
				else character = (char) std::tolower((unsigned char) character);
			}
			return text;
		}

		Scenario base(const std::string& group, const std::string& input, BrushType brush, halnf size) {
			Scenario scenario;
			scenario.group = group;
			scenario.input = input;
			scenario.brush = brush;
			scenario.size = size;
			return scenario;
		}

		void addSizeSweep(std::vector<Scenario>& scenarios, const std::string& input) {
			for (halnf size : { 4.0f, 20.0f, 80.0f, 250.0f }) {
				Scenario scenario = base("brush size", input, BrushType::Round, size);
				scenario.name = "round size " + number(size);
				scenarios.push_back(scenario);
			}
		}

		void addZoomSweep(std::vector<Scenario>& scenarios, const std::string& input) {
			for (halnf zoom : { 0.25f, 1.0f, 4.0f }) {
				Scenario scenario = base("zoom", input, BrushType::Round, 20);
				scenario.zoom = zoom;
				scenario.name = "round zoom " + number(zoom) + "x";
				scenarios.push_back(scenario);
			}
		}

		void addPresetSweep(std::vector<Scenario>& scenarios, const std::string& input) {
			for (BrushType type : { BrushType::Ink,
															BrushType::Pencil,
															BrushType::Marker,
															BrushType::Calligraphy,
															BrushType::Jagged,
															BrushType::Flat,
															BrushType::Air }) {
				Scenario scenario = base("brush preset", input, type, 40);
				scenario.name = slug(presetLabel(type)) + " size 40";
				scenarios.push_back(scenario);
			}
		}

		void addParameterSweep(std::vector<Scenario>& scenarios, const std::string& input) {
			Scenario textured = base("brush parameters", input, BrushType::Round, 60);
			textured.name = "round textured";
			textured.tune = [](Brush& brush) { brush.set(BrushParameter::Texture, 1); };
			scenarios.push_back(textured);

			Scenario dense = base("brush parameters", input, BrushType::Round, 60);
			dense.name = "round spacing 1%";
			dense.tune = [](Brush& brush) { brush.setSpacing(0.01f); };
			scenarios.push_back(dense);

			Scenario soft = base("brush parameters", input, BrushType::Round, 60);
			soft.name = "round soft half opacity";
			soft.tune = [](Brush& brush) {
				brush.setHardness(0);
				brush.setOpacity(0.5f);
			};
			scenarios.push_back(soft);
		}

		void addStabilizerSweep(std::vector<Scenario>& scenarios, const std::string& input) {
			for (halnf amount : { 0.0f, 0.325f, 0.7f, 1.0f }) {
				Scenario scenario = base("stabilizer", input, BrushType::Round, 20);
				scenario.stabilizer = amount;
				scenario.name = "round stabilizer " + number(amount);
				scenarios.push_back(scenario);
			}
		}

		void addEraserSweep(std::vector<Scenario>& scenarios, const std::string& input) {
			for (halnf size : { 20.0f, 120.0f }) {
				Scenario scenario = base("eraser", input, BrushType::Eraser, size);
				scenario.prepaint = true;
				scenario.name = "eraser size " + number(size);
				scenarios.push_back(scenario);
			}
			Scenario zoomed = base("eraser", input, BrushType::Eraser, 40);
			zoomed.prepaint = true;
			zoomed.zoom = 0.25f;
			zoomed.name = "eraser zoom 0.25x";
			scenarios.push_back(zoomed);

			Scenario mode = base("eraser", input, BrushType::Calligraphy, 40);
			mode.prepaint = true;
			mode.eraserMode = true;
			mode.name = "calligraphy eraser mode";
			scenarios.push_back(mode);
		}
	}

	std::string Scenario::describe() const {
		if (session) {
			const SessionSnapshot& recorded = *session;
			return "recorded session from " + recorded.environment.date + " on " + recorded.environment.device + " at " +
						 recorded.environment.revision + ": " + presetLabel(recorded.brush.type) + ", size " +
						 number(recorded.brush.size()) + ", zoom " + number(recorded.view.zoom) + "x, " +
						 std::to_string(recorded.document.layers.size()) + " layers";
		}
		std::string text =
			presetLabel(brush) + ", size " + number(size) + ", zoom " + number(zoom) + "x, stabilizer " + number(stabilizer);
		if (eraserMode) text += ", eraser mode";
		if (prepaint) text += ", on painted layer";
		return text;
	}

	std::vector<Scenario> brushAndEraserScenarios(const std::vector<std::string>& inputs) {
		std::vector<Scenario> scenarios;
		for (const std::string& input : inputs) {
			const size_t first = scenarios.size();
			addSizeSweep(scenarios, input);
			addZoomSweep(scenarios, input);
			addPresetSweep(scenarios, input);
			addParameterSweep(scenarios, input);
			addStabilizerSweep(scenarios, input);
			addEraserSweep(scenarios, input);
			if (inputs.size() < 2) continue;
			for (size_t index = first; index < scenarios.size(); index++)
				scenarios[index].name = input + " / " + scenarios[index].name;
		}
		return scenarios;
	}
}

#pragma once

#include "Animations.hpp"
#include "Map.hpp"
#include "Rect.hpp"
#include "Strings.hpp"

namespace tp {

	extern ModuleManifest gModuleWidgets;

	struct WidgetConfig {
		struct Node {
			enum Type { NONE, VAL, COL, REF };

			halnf value = 0.f;
			RGBA color = {};

			Type type = NONE;

			String refGroup;
			String refName;

			Node() = default;

			explicit Node(halnf val) {
				type = VAL;
				value = val;
			}

			explicit Node(const RGBA& val) {
				type = COL;
				color = val;
			}

			explicit Node(const String& val) {
				refGroup = "Presets";
				refName = val;
				type = REF;
			}
		};

		Map<String, Node> values;
	};

	struct GlobalGUIConfig {
		GlobalGUIConfig() {
			configs.put("Presets", {});
			auto& presets = configs.get("Presets");

			presets.values.put("FontSize", WidgetConfig::Node(15.f));
			presets.values.put("FontSizeDim", WidgetConfig::Node(12.f));
			presets.values.put("Rounding", WidgetConfig::Node(5.f));
			presets.values.put("Padding", WidgetConfig::Node(5.f));
			presets.values.put("HandleSize", WidgetConfig::Node(5.f));

			presets.values.put("Background", WidgetConfig::Node({ 0.03f, 0.03f, 0.03f, 1.f }));
			presets.values.put("Base", WidgetConfig::Node({ 0.07f, 0.07f, 0.07f, 1.f }));
			presets.values.put("Accent", WidgetConfig::Node({ 0.13f, 0.13f, 0.13f, 1.f }));
			presets.values.put("Interaction", WidgetConfig::Node({ 0.33f, 0.33f, 0.3f, 1.f }));
			presets.values.put("Action", WidgetConfig::Node({ 0.44f, 0.44f, 0.4f, 1.f }));
			presets.values.put("Front", WidgetConfig::Node({ 1.f, 1.f, 1.f, 1.f }));
			presets.values.put("FrontDim", WidgetConfig::Node({ 0.7f, 0.7f, 0.7f, 1.f }));
		}

		Map<String, WidgetConfig> configs;
	};

	extern GlobalGUIConfig* gGlobalGUIConfig;
}
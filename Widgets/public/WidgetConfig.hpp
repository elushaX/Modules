#pragma once

#include "Animations.hpp"
#include "Map.hpp"
#include "Rect.hpp"

#include "InputCodes.hpp"
#include "Buffer.hpp"

namespace tp {

	struct WidgetShortcut {
		struct Condition {
			std::string name;
			std::string state;
		};

		WidgetShortcut() = default;
		WidgetShortcut(const InitialierList<Condition>&) {}
	};

	template <typename Events, typename Canvas>
	struct WidgetConfig {

		struct WidgetCallback {
			using CallbackType = void (*)(void* self, const Events&);
			void* self;
			CallbackType callback;
		};

		struct Node {
			enum Type { NONE, VAL, COL, REF };

			halnf value = 0.f;
			RGBA color = {};

			Type type = NONE;

			std::string refGroup;
			std::string refName;

			Node() = default;

			explicit Node(halnf val) {
				type = VAL;
				value = val;
			}

			explicit Node(const RGBA& val) {
				type = COL;
				color = val;
			}

			explicit Node(const std::string& val) {
				refGroup = "Presets";
				refName = val;
				type = REF;
			}
		};

		Map<std::string, Node> values;
		Map<std::string, WidgetCallback> mOperators;
		Map<std::string, Buffer<WidgetShortcut>> mShortcuts;
	};

	template <typename Events, typename Canvas>
	struct GlobalGUIConfig {
		using WidgetConfig = WidgetConfig<Events, Canvas>;
		using WidgetNode = WidgetConfig::Node;

		GlobalGUIConfig() {
			configs.put("Presets", {});
			auto& presets = configs.get("Presets");

			presets.values.put("FontSize", WidgetNode(15.f));
			presets.values.put("FontSizeDim", WidgetNode(12.f));
			presets.values.put("Rounding", WidgetNode(5.f));
			presets.values.put("Padding", WidgetNode(5.f));
			presets.values.put("HandleSize", WidgetNode(5.f));

			presets.values.put("Background", WidgetNode(RGBA{ 0.03f, 0.03f, 0.03f, 1.f }));
			presets.values.put("Base", WidgetNode(RGBA{ 0.07f, 0.07f, 0.07f, 1.f }));
			presets.values.put("Accent", WidgetNode(RGBA{ 0.13f, 0.13f, 0.13f, 1.f }));
			presets.values.put("Interaction", WidgetNode(RGBA{ 0.33f, 0.33f, 0.3f, 1.f }));
			presets.values.put("Action", WidgetNode(RGBA{ 0.44f, 0.44f, 0.4f, 1.f }));
			presets.values.put("Front", WidgetNode(RGBA{ 1.f, 1.f, 1.f, 1.f }));
			presets.values.put("FrontDim", WidgetNode(RGBA{ 0.7f, 0.7f, 0.7f, 1.f }));
		}

		~GlobalGUIConfig() { configs.removeAll(); }

		void processShortcuts(const Events& events) {
			//
		}

		Map<std::string, WidgetConfig> configs;
	};
}
#pragma once

#include "WidgetConfig.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class Widget {
	public:
		Widget() = default;

		[[nodiscard]] const RGBA& getColor(const String& name) const {
			const auto& node = cfg->values.get(name);
			if (node.type == WidgetConfig::Node::REF) {
				return globalCfg->configs.get(node.refGroup).values.get(node.refName).color;
			}
			return cfg->values.get(name).color;
		}

		[[nodiscard]] halnf getValue(const String& name) const {
			const auto& node = cfg->values.get(name);
			if (node.type == WidgetConfig::Node::REF) {
				return globalCfg->configs.get(node.refGroup).values.get(node.refName).value;
			}
			return cfg->values.get(name).value;
		}

		void addColor(const String& name, const RGBA& val) { cfg->values.put(name, WidgetConfig::Node(val)); }
		void addValue(const String& name, halnf val) { cfg->values.put(name, WidgetConfig::Node(val)); }

		void addColor(const String& name, const String& presetName) {
			cfg->values.put(name, WidgetConfig::Node(presetName));
		}
		void addValue(const String& name, const String& presetName) {
			cfg->values.put(name, WidgetConfig::Node(presetName));
		}

		void createConfig(const String& name) {
			globalCfg->configs.put(name, {});
			cfg = &globalCfg->configs.get(name);
		}

		virtual void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) {
			mVisible = areaParent.isOverlap(aArea);
			if (!mVisible) {
				return;
			}

			this->mArea = aArea;
		}

		virtual void draw(Canvas& canvas) {
			if (!mVisible) {
				return;
			}
		}

	public:
		GlobalGUIConfig* globalCfg = gGlobalGUIConfig;
		WidgetConfig* cfg = nullptr;
		RectF mArea;
		bool mVisible = false;
	};
}
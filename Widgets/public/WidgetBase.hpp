;
#pragma once

#include "WidgetConfig.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class Widget {
	public:
		using WidgetConfig = WidgetConfig<Events, Canvas>;
		using WidgetNode = WidgetConfig::Node;
		using GlobalGUIConfig = GlobalGUIConfig<Events, Canvas>;

		Widget() { this->mArea = { 0, 0, 100, 100 }; }

		virtual void populateConfig() {}

		[[nodiscard]] const RGBA& getColor(const std::string& name) const {
			const auto& node = cfg->values.get(name);
			if (node.type == WidgetNode::REF) {
				return mGlobalCfg.configs.get(node.refGroup).values.get(node.refName).color;
			}
			return cfg->values.get(name).color;
		}

		[[nodiscard]] halnf getValue(const std::string& name) const {
			const auto& node = cfg->values.get(name);
			if (node.type == WidgetNode::REF) {
				return mGlobalCfg.configs.get(node.refGroup).values.get(node.refName).value;
			}
			return cfg->values.get(name).value;
		}

		void addColor(const std::string& name, const RGBA& val) { cfg->values.put(name, WidgetNode(val)); }
		void addValue(const std::string& name, halnf val) { cfg->values.put(name, WidgetNode(val)); }

		void addColor(const std::string& name, const std::string& presetName) {
			cfg->values.put(name, WidgetNode(presetName));
		}
		void addValue(const std::string& name, const std::string& presetName) {
			cfg->values.put(name, WidgetNode(presetName));
		}

		bool createConfig(const std::string& name) {
			auto idx = mGlobalCfg.configs.presents(name);
			if (idx) return false;
			mGlobalCfg.configs.put(name, {});
			cfg = &mGlobalCfg.configs.get(name);
			populateConfig();
			return true;
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

		void addOperator(const std::string& name, WidgetConfig::WidgetCallback callback) {
			cfg->mOperators.put(name, callback);
		}

		Buffer<WidgetShortcut>& getShortcuts(const std::string& name) { return cfg->mShortcuts[name]; }

	public:
		static GlobalGUIConfig mGlobalCfg;
		WidgetConfig* cfg = nullptr;
		RectF mArea;
		bool mVisible = false;
	};

	template <typename Events, typename Canvas>
	GlobalGUIConfig<Events, Canvas> Widget<Events, Canvas>::mGlobalCfg;
}
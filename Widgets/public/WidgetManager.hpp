#pragma once

#include "Animations.hpp"
#include "Map.hpp"
#include "Rect.hpp"

#include "InputCodes.hpp"
#include "Buffer.hpp"

namespace tp {

	struct WidgetConfig {

		struct WidgetShortcut {
			struct Condition {
				std::string name;
				std::string state;
			};

			std::string callbackName;

			WidgetShortcut() = default;
			WidgetShortcut(const InitialierList<Condition>&) {}
		};

		struct WidgetParameter {
			enum Type { NONE, VAL, COL, REF };

			halnf value = 0.f;
			RGBA color = {};

			std::string refName;
			std::string refWidgetId;

			Type type = NONE;

			WidgetParameter() = default;

			explicit WidgetParameter(halnf val) {
				type = VAL;
				value = val;
			}

			explicit WidgetParameter(const RGBA& val) {
				type = COL;
				color = val;
			}

			explicit WidgetParameter(const std::string& widgetId, const std::string& val) {
				type = REF;
				refName = val;
				refWidgetId = widgetId;
			}
		};

		Map<std::string, WidgetParameter> mParameters;
		Buffer<WidgetShortcut> mShortcuts;
	};

	class WidgetManager {
	public:
		WidgetManager() {
			createWidgetConfig("Default");

			addNumber("Default", "FontSize", 15.f);
			addNumber("Default", "FontSizeDim", 12.f);
			addNumber("Default", "Rounding", 5.f);
			addNumber("Default", "Padding", 5.f);
			addNumber("Default", "HandleSize", 5.f);

			addColor("Default", "Background", RGBA{ 0.03f, 0.03f, 0.03f, 1.f });
			addColor("Default", "Base", RGBA{ 0.07f, 0.07f, 0.07f, 1.f });
			addColor("Default", "Accent", RGBA{ 0.13f, 0.13f, 0.13f, 1.f });
			addColor("Default", "Interaction", RGBA{ 0.33f, 0.33f, 0.3f, 1.f });
			addColor("Default", "Action", RGBA{ 0.44f, 0.44f, 0.4f, 1.f });
			addColor("Default", "Front", RGBA{ 1.f, 1.f, 1.f, 1.f });
			addColor("Default", "FrontDim", RGBA{ 0.7f, 0.7f, 0.7f, 1.f });
		}

		~WidgetManager() { mConfigurations.removeAll(); }

		bool createWidgetConfig(const std::string& widgetId) {
			auto idx = mConfigurations.presents(widgetId);
			if (idx) return false;
			mConfigurations.put(widgetId, {});
			return true;
		}

		[[nodiscard]] const RGBA& getColor(const std::string& widgetId, const std::string& name) const {
			const WidgetConfig& config = mConfigurations.get(widgetId);
			const WidgetConfig::WidgetParameter& parameter = config.mParameters.get(name);

			if (parameter.type == WidgetConfig::WidgetParameter::REF) {
				return mConfigurations.get(parameter.refWidgetId).mParameters.get(parameter.refName).color;
			} else {
				return parameter.color;
			}
		}

		[[nodiscard]] halnf getNumber(const std::string& widgetId, const std::string& name) const {
			const WidgetConfig& config = mConfigurations.get(widgetId);
			const WidgetConfig::WidgetParameter& parameter = config.mParameters.get(name);

			if (parameter.type == WidgetConfig::WidgetParameter::REF) {
				return mConfigurations.get(parameter.refWidgetId).mParameters.get(parameter.refName).value;
			} else {
				return parameter.value;
			}
		}

		void addColor(const std::string& widgetId, const std::string& name, const RGBA& val) {
			WidgetConfig& config = mConfigurations.get(widgetId);
			config.mParameters.put(name, WidgetConfig::WidgetParameter(val));
		}

		void addNumber(const std::string& widgetId, const std::string& name, halnf val) {
			WidgetConfig& config = mConfigurations.get(widgetId);
			config.mParameters.put(name, WidgetConfig::WidgetParameter(val));
		}

		void addReference(const std::string& widgetId, const std::string& refName, const std::string& name) {
			WidgetConfig& config = mConfigurations.get(widgetId);
			config.mParameters.put(refName, WidgetConfig::WidgetParameter("Default", name));
		}

	private:
		Map<std::string, WidgetConfig> mConfigurations;
		RGBA mErrorColor = { 0, 0, 0, 1 };
		halnf mErrorNumber = 0;
	};
}
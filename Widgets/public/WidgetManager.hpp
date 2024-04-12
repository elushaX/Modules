#pragma once

#include "Animations.hpp"
#include "Map.hpp"
#include "Rect.hpp"

#include "InputCodes.hpp"
#include "Buffer.hpp"

namespace tp {

	struct WidgetConfig {

		struct Shortcut {
			struct Condition {
				std::string name;
				std::string state;
			};

			std::string callbackName;

			Shortcut() = default;
			Shortcut(const InitialierList<Condition>&) {}
		};

		struct Parameter {
			enum Type { NONE, VAL, COL };

			halnf value = 0.f;
			RGBA color = {};
			Type type = NONE;
			bool globalReference = false;
			std::string globalId;

			Parameter() = default;

			explicit Parameter(halnf val) {
				type = VAL;
				value = val;
			}

			explicit Parameter(const RGBA& val) {
				type = COL;
				color = val;
			}

			explicit Parameter(const std::string& id, const Type& referenceType) {
				type = referenceType;
				globalId = id;
				globalReference = true;
			}
		};

		Map<std::string, Parameter> mParameters;
		Buffer<Shortcut> mShortcuts;
	};

	class WidgetManager {
	public:
		using Parameter = WidgetConfig::Parameter;

	public:
		WidgetManager() { initGlobalParameters(); }
		~WidgetManager() { mConfigurations.removeAll(); }

		const RGBA& getColor(const std::string& parameterId, const RGBA& defaultValue) {
			WidgetConfig& config = getWidgetConfig(mActiveId);
			Parameter& parameter = getParameter(config, parameterId, Parameter(defaultValue));
			return parameter.color;
		}

		const RGBA& getColor(const std::string& parameterId, const char* globalRef) {
			WidgetConfig& config = getWidgetConfig(mActiveId);
			Parameter& parameter = getParameter(config, parameterId, Parameter(globalRef, Parameter::COL));
			return parameter.color;
		}

		halnf getNumber(const std::string& parameterId, halnf defaultValue) {
			WidgetConfig& config = getWidgetConfig(mActiveId);
			Parameter& parameter = getParameter(config, parameterId, Parameter(defaultValue));
			return parameter.value;
		}

		halnf getNumber(const std::string& parameterId, const char* globalRef) {
			WidgetConfig& config = getWidgetConfig(mActiveId);
			Parameter& parameter = getParameter(config, parameterId, Parameter(globalRef, Parameter::VAL));
			return parameter.value;
		}

		void setActiveId(const std::string& id) { mActiveId = id; }
		
	private:
		WidgetConfig& getWidgetConfig(const std::string& id) {
			auto idx = mConfigurations.presents(id);
			if (idx) return mConfigurations.getSlotVal(idx);
			mConfigurations.put(id, {});
			return mConfigurations.get(id);
		}

		Parameter& getParameter(WidgetConfig& config, const std::string& id, const Parameter& defaultValue) {
			auto idx = config.mParameters.presents(id);

			if (!idx) {
				config.mParameters.put(id, defaultValue);
			}

			Parameter& parameter = config.mParameters.get(id);

			if (parameter.globalReference) {
				return mGlobalConfig.mParameters.get(parameter.globalId);
			} else {
				return parameter;
			}
		}

		void initGlobalParameters() {
			auto& params = mGlobalConfig.mParameters;

			params.put("FontSize", Parameter(15.f));

			params.put("FontSizeDim", Parameter(12.f));
			params.put("Rounding", Parameter(5.f));
			params.put("Padding", Parameter(5.f));
			params.put("HandleSize", Parameter(5.f));

			params.put("Background", Parameter(RGBA{ 0.03f, 0.03f, 0.03f, 1.f }));
			params.put("Base", Parameter(RGBA{ 0.07f, 0.07f, 0.07f, 1.f }));
			params.put("Accent", Parameter(RGBA{ 0.13f, 0.13f, 0.13f, 1.f }));
			params.put("Interaction", Parameter(RGBA{ 0.33f, 0.33f, 0.3f, 1.f }));
			params.put("Action", Parameter(RGBA{ 0.44f, 0.44f, 0.4f, 1.f }));
			params.put("Front", Parameter(RGBA{ 1.f, 1.f, 1.f, 1.f }));
			params.put("FrontDim", Parameter(RGBA{ 0.7f, 0.7f, 0.7f, 1.f }));
		}

	private:
		Map<std::string, WidgetConfig> mConfigurations;
		WidgetConfig mGlobalConfig;

		RGBA mErrorColor = { 0, 0, 0, 1 };
		halnf mErrorNumber = 0;

		std::string mActiveId;
	};
}
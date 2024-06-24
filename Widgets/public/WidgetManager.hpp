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
		WidgetManager();
		~WidgetManager();

		const RGBA& getColor(const std::string& parameterId, const RGBA& defaultValue);
		const RGBA& getColor(const std::string& parameterId, const char* globalRef);

		halnf getNumber(const std::string& parameterId, halnf defaultValue);
		halnf getNumber(const std::string& parameterId, const char* globalRef);

		void setActiveId(const std::string& id);
		
	private:
		WidgetConfig& getWidgetConfig(const std::string& id);
		Parameter& getParameter(WidgetConfig& config, const std::string& id, const Parameter& defaultValue);

		void initGlobalParameters();

	private:
		Map<std::string, WidgetConfig> mConfigurations;
		WidgetConfig mGlobalConfig;

		RGBA mErrorColor = { 0, 0, 0, 1 };
		halnf mErrorNumber = 0;

		std::string mActiveId;
	};
}
#include "WidgetManager.hpp"

using namespace tp;

WidgetManager::WidgetManager() { initGlobalParameters(); }
WidgetManager::~WidgetManager() { mConfigurations.removeAll(); }

const RGBA& WidgetManager::getColor(const std::string& parameterId, const RGBA& defaultValue) {
	WidgetConfig& config = getWidgetConfig(mActiveId);
	Parameter& parameter = getParameter(config, parameterId, Parameter(defaultValue));
	return parameter.color;
}

const RGBA& WidgetManager::getColor(const std::string& parameterId, const char* globalRef) {
	WidgetConfig& config = getWidgetConfig(mActiveId);
	Parameter& parameter = getParameter(config, parameterId, Parameter(globalRef, Parameter::COL));
	return parameter.color;
}

halnf WidgetManager::getNumber(const std::string& parameterId, halnf defaultValue) {
	WidgetConfig& config = getWidgetConfig(mActiveId);
	Parameter& parameter = getParameter(config, parameterId, Parameter(defaultValue));
	return parameter.value;
}

halnf WidgetManager::getNumber(const std::string& parameterId, const char* globalRef) {
	WidgetConfig& config = getWidgetConfig(mActiveId);
	Parameter& parameter = getParameter(config, parameterId, Parameter(globalRef, Parameter::VAL));
	return parameter.value;
}

void WidgetManager::setActiveId(const std::string& id) { mActiveId = id; }

WidgetConfig& WidgetManager::getWidgetConfig(const std::string& id) {
	auto idx = mConfigurations.presents(id);
	if (idx) return mConfigurations.getSlotVal(idx);
	mConfigurations.put(id, {});
	return mConfigurations.get(id);
}

WidgetManager::Parameter& WidgetManager::getParameter(WidgetConfig& config, const std::string& id, const Parameter& defaultValue) {
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

void WidgetManager::initGlobalParameters() {
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

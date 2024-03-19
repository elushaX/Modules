
#include "GraphicApplication.hpp"
#include "imgui.h"

using namespace tp;

class ExampleApplication : public Application {
public:
	ExampleApplication() = default;

	void processFrame(EventHandler* eventHandler) override { 
		// example
	}

	virtual void drawFrame(Canvas* canvas) override {
		ImGui::Text("Frames processed per second: %f", this->mFramesProcessedPerSecond);
		ImGui::Text("Frames drawn per second: %f", this->mFramesDrawnPerSecond);
	}

	virtual ~ExampleApplication() = default;
};

int main() { 

	ModuleManifest* deps[] = { &gModuleGraphics, nullptr };
	ModuleManifest module("Eample", nullptr, nullptr, deps);

	if (!module.initialize()) {
		return 1;
	}

	{
		ExampleApplication app;
		app.run();
	}

	module.deinitialize();
}

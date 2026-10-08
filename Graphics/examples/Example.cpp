
#include "GraphicApplication.hpp"

#include "imgui.h"

using namespace tp;

class ExampleApplication : public Application {
public:
	ExampleApplication() = default;

	void processFrame(EventHandler* eventHandler, halnf) override {
		// example
	}

	virtual void drawFrame(Canvas* canvas) override {
		ImGui::ShowDemoWindow();

		drawDebug();
	}

	bool forceNewFrame() override { return false; }

	virtual ~ExampleApplication() = default;
};

int main() { 
	ExampleApplication app;
	app.run();
}


#include "GraphicApplication.hpp"
#include <GL/glew.h>


#include "imgui.h"

using namespace tp;

class ExampleApplication : public Application {
public:
	ExampleApplication() = default;

	void processFrame(EventHandler* eventHandler, halnf) override {
		// example
	}

	virtual void drawFrame(Canvas* canvas) override {
		glClear(GL_COLOR_BUFFER_BIT);

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


#include "GraphicApplication.hpp"
#include <GL/glew.h>


#include "imgui.h"

using namespace tp;

class ExampleApplication : public Application {
public:
	ExampleApplication() = default;

	void processFrame(EventHandler* eventHandler) override { 
		// example
	}

	virtual void drawFrame(Canvas* canvas) override {
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui::ShowDemoWindow();

		ImGui::Text("Frames processed per second: %f", this->mFramesProcessedPerSecond);
		ImGui::Text("Frames drawn per second: %f", this->mFramesDrawnPerSecond);
	}

	virtual ~ExampleApplication() = default;
};

int main() { 
	ExampleApplication app;
	app.run();
}

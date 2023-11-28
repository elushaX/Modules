
#include "Window.hpp"
#include "Timing.hpp"

#include "imgui.h"

int main() {
	tp::ModuleManifest* deps[] = { &tp::gModuleGraphics, nullptr };
	tp::ModuleManifest testModule("Example", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	int frames = 0;
	int fps = 0;
	tp::Timer timer(1000);

	{
		auto window = tp::Window::createWindow(800, 600, "Window 1");

		if (window) {
			while (!window->shouldClose()) {
				window->processEvents();

				ImGui::Text("fps: %i", fps);

				window->draw();

				if (timer.isTimeout()) {
					fps = frames;
					frames = 0;
					timer.reset();
				}

				frames++;
			}
		}

		tp::Window::destroyWindow(window);
	}

	testModule.deinitialize();
}

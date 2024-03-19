
#include "Window.hpp"
#include "Graphics.hpp"

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

	auto window = tp::Window::createWindow();

	{
		tp::Graphics graphics(window);

		if (window) {
			while (!window->shouldClose()) {
				window->processEvents();
				graphics.proc();

				ImGui::Text("fps: %i", fps);

				graphics.draw();
				window->draw();

				if (timer.isTimeout()) {
					fps = frames;
					frames = 0;
					timer.reset();
				}

				frames++;
			}
		}

	}

	tp::Window::destroyWindow(window);

	testModule.deinitialize();
}

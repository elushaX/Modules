
#include "ClientGui.hpp"
#include "Window.hpp"

using namespace tp;

int main() {

	ModuleManifest* deps[] = { &tp::gModuleChat, nullptr };
	ModuleManifest testModule("ClientGUI", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	{
		ChatAPI chat;
		ClientGUI gui;

		auto window = Window::createWindow(1000, 700, "Window 1");

		if (window) {

			window->getGraphics().getDebugGui().setFontSize(4);

			while (!window->shouldClose()) {
				window->processEvents();

				gui.draw(chat);

				window->draw();
			}
		}

		Window::destroyWindow(window);
	}

	testModule.deinitialize();
}

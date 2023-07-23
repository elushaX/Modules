
#include "Graphics.hpp"

int main() {
	tp::ModuleManifest* deps[] = { &tp::gModuleAllocators, nullptr };
	tp::ModuleManifest testModule("Example", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	{
		tp::HeapAllocGlobal::startIgnore();
		auto window = tp::Window::createWindow(800, 600, "Window 1");
		tp::HeapAllocGlobal::stopIgnore();

		if (window) {
			window->renderLoop();
		}

		tp::Window::destroyWindow(window);
	}

	testModule.deinitialize();
}

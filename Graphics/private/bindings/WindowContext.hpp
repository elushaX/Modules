#pragma once

// -------- Window Context -------- //
#include <GLFW/glfw3.h>

namespace tp {
	class Window;

	class Window::Context {
		friend Graphics::GL;
		friend Graphics::GUI;
		friend Graphics::Canvas;

	public:
		GLFWwindow* window;
	};
}
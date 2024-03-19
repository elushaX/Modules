#pragma once

#include "Window.hpp"

class GLFWwindow;

namespace tp {

	class Window::Context {
	public:
		Context() = default;
		GLFWwindow* window = nullptr;
	};
}
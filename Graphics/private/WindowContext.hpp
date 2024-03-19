#pragma once

#include "Window.hpp"

struct GLFWwindow;

namespace tp {

	class Window::Context {
	public:
		Context() = default;
		GLFWwindow* window = nullptr;
	};
}
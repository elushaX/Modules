#pragma once

#include "Environment.hpp"

// -------- Window Context -------- //
#include <GLFW/glfw3.h>

#ifdef ENV_OS_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#ifdef ENV_OS_LINUX
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3native.h>

#include <array>

namespace tp {

	class Window;

	class InputManager {
	public:
		InputManager() = default;

		void init(GLFWwindow* aWindow) {
			window = aWindow;
			// Set up key and mouse button callbacks

			glfwSetWindowUserPointer(window, this);
			glfwSetKeyCallback(window, keyCallback);
			glfwSetMouseButtonCallback(window, mouseButtonCallback);
			glfwSetScrollCallback(window, scrollCallback);
		}

		const Vec2F& getPos() const { return mousePos; }

		bool isKeyPressed(int key) const { return keyStates[key] == GLFW_PRESS; }

		bool isKeyDown(int key) const { return keyStates[key] == GLFW_REPEAT || keyStates[key] == GLFW_PRESS; }

		bool isPressed() const { return mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == GLFW_PRESS; }

		bool isReleased() const { return mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == GLFW_RELEASE; }

		bool isDown() const { return mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == GLFW_REPEAT || mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == GLFW_PRESS; }

		bool isEvents() const { return isEvent; }

		double getScrollY() const { return scrollY; }

		// bool isScrollDown() const { return scrollY < 0.0; }

	private:
		friend Window;

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
			InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

			if (!inputManager) {
				// Update key state
				if (action == GLFW_PRESS) {
					inputManager->keyStates[key] = GLFW_PRESS;
				} else if (action == GLFW_RELEASE) {
					inputManager->keyStates[key] = GLFW_RELEASE;
				}
				inputManager->isEvent = true;
			}
		}

		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
			InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

			if (inputManager) {
				// Update mouse button state
				inputManager->mouseButtonStates[button] = action;
				inputManager->isEvent = true;
			}
		}

		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
			InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

			if (inputManager) {
				// Update scroll state
				inputManager->scrollX = xoffset;
				inputManager->scrollY = yoffset;
				inputManager->isEvent = true;
			}
		}

		void update() {
			double x, y;
			glfwGetCursorPos(window, &x, &y);
			mousePosPrev = mousePos;
			mousePos = { x, y };

			if ((mousePosPrev - mousePos).length2()) {
				isEvent = true;
			}
		}

		void resetScroll() {
			scrollX = 0.0;
			scrollY = 0.0;
			mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] = GLFW_REPEAT;
		}

		bool isEvent = false;
		double scrollX = 0.0;
		double scrollY = 0.0;
		GLFWwindow* window;
		Vec2F mousePos;
		Vec2F mousePosPrev;
		std::array<int, GLFW_KEY_LAST + 1> keyStates = { GLFW_RELEASE };
		std::array<int, GLFW_MOUSE_BUTTON_LAST + 1> mouseButtonStates = { GLFW_RELEASE };
	};

	class Window::Context {
		friend Graphics::GL;
		friend Graphics::GUI;
		friend Graphics::Canvas;

	public:
		GLFWwindow* window;
		InputManager inputManager;
	};
}
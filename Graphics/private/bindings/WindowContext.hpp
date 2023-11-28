#pragma once

// -------- Window Context -------- //
#include <GLFW/glfw3.h>
#include <array>

namespace tp {

	class Window;

	class InputManager {
	public:
		InputManager() = default;
		
		void init(GLFWwindow* aWindow)  {
			window = aWindow;
			// Set up key and mouse button callbacks
			glfwSetKeyCallback(window, keyCallback);
			glfwSetMouseButtonCallback(window, mouseButtonCallback);
			glfwSetScrollCallback(window, scrollCallback);
		}

		const Vec2F& getPos() const { return mousePos; }

		bool isKeyPressed(int key) const {
			return keyStates[key] == GLFW_PRESS;
		}

		bool isKeyDown(int key) const { 
			return keyStates[key] == GLFW_REPEAT || keyStates[key] == GLFW_PRESS;
		}

		bool isPressed() const { 
			return mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == GLFW_PRESS; 
		}

		bool isDown() const { 
			return mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == GLFW_REPEAT || mouseButtonStates[GLFW_MOUSE_BUTTON_LEFT] == GLFW_PRESS; 
		}

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
			}
		}

		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
			InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

			if (inputManager) {
				// Update mouse button state
				inputManager->mouseButtonStates[button] = action;
			}
		}

		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
			InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));

			if (inputManager) {
				// Update scroll state
				inputManager->scrollX = xoffset;
				inputManager->scrollY = yoffset;
			}
		}

		void update() {
			double x, y;
			glfwGetCursorPos(window, &x, &y);
			mousePos = { x, y };
		}

		void resetScroll() {
			scrollX = 0.0;
			scrollY = 0.0;
		}

		double scrollX = 0.0;
    double scrollY = 0.0;
		GLFWwindow* window;
		Vec2F mousePos;
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
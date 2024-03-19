
#include "Window.hpp"
#include "WindowContext.hpp"
#include "Allocators.hpp"

#include <GL/glew.h>

#ifdef ENV_OS_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#ifdef ENV_OS_LINUX
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3.h>
#include <cstdio>

namespace tp {
  static ModuleManifest* deps[] = { &gModuleAllocators, nullptr };
  ModuleManifest gModuleGraphics = ModuleManifest("Graphics", nullptr, nullptr, deps);
}

using namespace tp;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

Window::Window(Vec2F size, const char* title) {
	mContext = new Context();

	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);

	// Create a window and OpenGL context
	mContext->window = glfwCreateWindow((int) size.x, (int) size.y, title, nullptr, nullptr);
	if (!mContext->window) {
		printf("Failed to create GLFW window\n");
		return;
	}

	glfwMakeContextCurrent(mContext->window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		return;
	}

	// set event bindings
	glfwSetWindowUserPointer(mContext->window, this);
	glfwSetKeyCallback(mContext->window, keyCallback);
	glfwSetMouseButtonCallback(mContext->window, mouseButtonCallback);
	glfwSetScrollCallback(mContext->window, scrollCallback);
}

Window::~Window() {
	glfwDestroyWindow(mContext->window);
	delete mContext;
}

Window* Window::createWindow(Vec2F size, const char* title) {
	HeapAllocGlobal::startIgnore();

	static int count = 1;
	if (!count) {
		printf("Window class is a singleton\n");
		return nullptr;
	}
	count--;

	// Initialize GLFW
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return nullptr;
	}

	// Set the GLFW error callback
	glfwSetErrorCallback([](int error, const char* description) { printf("GLFW Error: %i %s\n", error, description); });

	auto out = new Window(size, title);

	HeapAllocGlobal::stopIgnore();
	return out;
}

void Window::destroyWindow(Window* window) {
	delete window;
	glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(mContext->window); }

void Window::processEvents() {
	glfwPollEvents();

	int w, h;
	glfwGetWindowSize(mContext->window, &w, &h);
	mSize = { (halnf) w, (halnf) h };
}

void Window::draw() {
	glfwSwapBuffers(mContext->window);
}

const Vec2F& Window::getSize() const { return mSize; }

Window::Context* Window::getContext() { return mContext; }

void Window::setEventHandler(EventHandler* eventHandler) { mEventHandler = eventHandler; }
EventHandler* Window::getEventHandler() { return mEventHandler; }


static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!self) return;

	EventHandler* eventHandler = self->getEventHandler();
	if (!eventHandler) return;

	// post key event
	/*
	if (action == GLFW_PRESS) {
		inputManager->keyStates[key] = GLFW_PRESS;
	} else if (action == GLFW_RELEASE) {
		inputManager->keyStates[key] = GLFW_RELEASE;
	}
	inputManager->isEvent = true;
	*/
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!self) return;

	EventHandler* eventHandler = self->getEventHandler();
	if (!eventHandler) return;

	// post mouse button event
	// inputManager->mouseButtonStates[button] = action;
	// inputManager->isEvent = true;
}

static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
	// ignore for now
}
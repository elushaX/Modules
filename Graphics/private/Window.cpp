
#include "Window.hpp"
#include "WindowContext.hpp"

#include "Rect.hpp"

#include <GL/glew.h>

#ifdef ENV_OS_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#ifdef ENV_OS_LINUX
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3.h>
#include <cstdio>

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

	return out;
}

void Window::destroyWindow(Window* window) {
	delete window;
	glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(mContext->window); }

void Window::processEvents() {
	glfwWaitEvents();
	checkAxisUpdates();
}

void Window::draw() {
	glfwSwapBuffers(mContext->window);
}

const Vec2F& Window::getSize() const { return mSize; }

Window::Context* Window::getContext() { return mContext; }

void Window::setEventHandler(EventHandler* eventHandler) { mEventHandler = eventHandler; }
EventHandler* Window::getEventHandler() { return mEventHandler; }


void Window::checkAxisUpdates() {
	if (!mEventHandler) return;

	int w, h;
	glfwGetWindowSize(mContext->window, &w, &h);

	if (mSize != Vec2F((halnf) w, (halnf) h)) {
		mSize = { (halnf) w, (halnf) h };
		mEventHandler->postEvent(InputID::WINDOW_RESIZE, {});
	}

	double x, y;
	glfwGetCursorPos(mContext->window, &x, &y);

	int posX, posY;
	glfwGetWindowPos(mContext->window, &posX, &posY);

	if (mPointerPos != Vec2F{ (halnf) x, (halnf) y }) {
		mPointerPos = { (halnf) x, (halnf) y };

		auto pos = Vec2F{ (halnf) posX, (halnf) posY };
		RectF windowRec = { { 0, 0 }, mSize };

		if (windowRec.isInside(mPointerPos)) {
			mEventHandler->postEvent(InputID::MOUSE1, { InputEvent::Type::MOUSE_POS, {}, mPointerPos });
		}
	}
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!self) return;

	EventHandler* eventHandler = self->getEventHandler();
	if (!eventHandler) return;

	self->checkAxisUpdates();

	if (action == GLFW_PRESS) {
		eventHandler->postEvent((InputID) key, { InputEvent::Type::BUTTON_ACTION, InputEvent::ButtonAction::PRESS, {} });
	} else if (action == GLFW_RELEASE) {
		eventHandler->postEvent((InputID) key, { InputEvent::Type::BUTTON_ACTION, InputEvent::ButtonAction::RELEASE, {} });
	}
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!self) return;

	EventHandler* eventHandler = self->getEventHandler();
	if (!eventHandler) return;

	self->checkAxisUpdates();

	auto id = (InputID) ((int) InputID::MOUSE1 + button);

	if (action == GLFW_PRESS) {
		eventHandler->postEvent(id, { InputEvent::Type::BUTTON_ACTION, InputEvent::ButtonAction::PRESS, {} }
		);
	} else if (action == GLFW_RELEASE) {
		eventHandler->postEvent(id, { InputEvent::Type::BUTTON_ACTION, InputEvent::ButtonAction::RELEASE, {} });
	}
}

static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
	// ignore for now
}
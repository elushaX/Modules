#include "Window.hpp"

// -------- OpenGL -------- //
#include <GL/glew.h>

#include "WindowContext.hpp"

#include <cstdio>

namespace tp {
	class Graphics::GL::Context {
	public:
		// Showoff data
		GLuint vao{};
		GLuint vbo{};
		GLfloat vertices[9] = {
			-0.0f, -0.0f, 0.0f, // Left vertex
			1.0f,  0.0f,  0.0f, // Right vertex
			0.5f,  1.0f,  0.0f  // Top vertex
		};
	};
}

using namespace tp;

Graphics::GL::GL() { mContext = new Context(); }

Graphics::GL::~GL() { delete mContext; }

void Graphics::GL::init() {
	// Create a Vertex Array Object (VAO)
	glGenVertexArrays(1, &mContext->vao);
	glBindVertexArray(mContext->vao);

	// Create a Vertex Buffer Object (VBO)
	glGenBuffers(1, &mContext->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mContext->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(mContext->vertices), mContext->vertices, GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*) nullptr);
	glEnableVertexAttribArray(0);
}

void Graphics::GL::deinit() {
	glDeleteBuffers(1, &mContext->vbo);
	glDeleteVertexArrays(1, &mContext->vao);
}

void Graphics::GL::proc() {
}

void Graphics::GL::draw() {
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, mWidth, mHeight);
	// glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Graphics::init(Window* window) {
	mGl.init();
	mGui.init(window);
	mCanvas.init();
}

void Graphics::deinit() {
	mCanvas.deinit();
	mGui.deinit();
	mGl.deinit();
}

void Graphics::proc() {
	mGl.proc();
	mCanvas.proc();
	mGui.proc();
}

void Graphics::draw() {
	mGl.draw();
	mCanvas.draw();
	mGui.draw();
}

Window::Window(int width, int height, const char* title) {
	mContext = new Context();

	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);

	// Create a window and OpenGL context
	mContext->window = glfwCreateWindow(width, height, title, nullptr, nullptr);
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

	mContext->inputManager.init(mContext->window);
	
	mGraphics.init(this);

	mEvents.mContext = mContext;
}

Window::~Window() {
	mGraphics.deinit();
	glfwDestroyWindow(mContext->window);
	delete mContext;
}

Window* Window::createWindow(int width, int height, const char* title) {
	tp::HeapAllocGlobal::startIgnore();

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

	auto out = new Window(width, height, title);

	tp::HeapAllocGlobal::stopIgnore();
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

	mGraphics.mCanvas.mWidth = w;
	mGraphics.mCanvas.mHeight = h;

	mGraphics.mGl.mWidth = w;
	mGraphics.mGl.mHeight = h;

	mGraphics.proc();

	mContext->inputManager.update();
	get_time();
}

void Window::draw() {
	mGraphics.draw();
	glfwSwapBuffers(mContext->window);
	mContext->inputManager.resetScroll();
}

auto Window::getContext() -> Context* { return mContext; }
Graphics::Canvas& Window::getCanvas() { return mGraphics.mCanvas; }
const Window::Events& Window::getEvents() { return mEvents; }

const Vec2F& Window::Events::getPos() const {
	return mContext->inputManager.getPos();
}

bool Window::Events::isPressed() const {
	return mContext->inputManager.isPressed();
}

bool Window::Events::isDown() const {
	return mContext->inputManager.isDown();
}

halnf Window::Events::getScrollY() const {
	return mContext->inputManager.getScrollY();
}
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
				1.0f, 0.0f, 0.0f,  // Right vertex
				0.5f, 1.0f, 0.0f     // Top vertex
		};
	};
}

using namespace tp;

Graphics::GL::GL() {
	mContext = new Context();
}

Graphics::GL::~GL() {
	delete mContext;
}

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
	glClear(GL_COLOR_BUFFER_BIT);
}

void Graphics::GL::draw() {
  tp::HeapAllocGlobal::startIgnore();
	glDrawArrays(GL_TRIANGLES, 0, 3);
  tp::HeapAllocGlobal::stopIgnore();
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

	// get some common information. maybe call it in loop to pick up monitor changes
	int width_mm, height_mm;
	auto monitor = glfwGetPrimaryMonitor();
	glfwGetMonitorPhysicalSize(monitor, &width_mm, &height_mm);
	mMonitor.mmSize = { width_mm, height_mm };

	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	mMonitor.pixelSize = { mode->width, mode->height };
	mMonitor.refreshRate = mode->refreshRate;

	mGraphics.init(this);
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
	glfwSetErrorCallback([] (int error, const char* description) {
		printf("GLFW Error: %i %s\n", error, description);
	});

	auto out = new Window(width, height, title);

	tp::HeapAllocGlobal::stopIgnore();
	return out;
}

void Window::destroyWindow(Window* window) {
	delete window;
	glfwTerminate();
}

bool Window::shouldClose() const {
	return glfwWindowShouldClose(mContext->window);
}

void Window::processEvents() {
	glfwPollEvents();
	mGraphics.proc();
}

void Window::draw() {
	mGraphics.draw();
	glfwSwapBuffers(mContext->window);
}

auto Window::getContext() -> Context* { return mContext; }
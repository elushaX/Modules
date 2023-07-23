#include "Graphics.hpp"

#include "Allocators.hpp"

// -------- OpenGL -------- //
#include <GL/glew.h>

// -------- Window Context -------- //
#include <GLFW/glfw3.h>

// -------- Debug UI -------- //
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// -------- Canvas -------- //
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>

#include <cstdio>

namespace tp {

	class ShowoffGlContext {
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

	class ShowoffGUIContext {
	public:
		ImGuiIO* io{};
		ImGuiContext* ctx{};
	};

	class ShowoffCanvasContext {
	public:
		NVGcontext* vg;
	};

	class WindowContext {
		friend Showoff::GL;
		friend Showoff::GUI;
		friend Showoff::Canvas;
	public:
		GLFWwindow* window;
	};
}

using namespace tp;

Showoff::GL::GL() {
	mContext = new ShowoffGlContext();
}

Showoff::GL::~GL() {
	delete mContext;
}

void Showoff::GL::init() {
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

void Showoff::GL::deinit() {
	glDeleteBuffers(1, &mContext->vbo);
	glDeleteVertexArrays(1, &mContext->vao);
}

void Showoff::GL::beginDraw() {
	glClear(GL_COLOR_BUFFER_BIT);
}

void Showoff::GL::endDraw() {
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

Showoff::GUI::GUI() {
	mContext = new ShowoffGUIContext();
}

Showoff::GUI::~GUI() {
	delete mContext;
}

void Showoff::GUI::init(Window* window) {
	IMGUI_CHECKVERSION();
	mContext->ctx = ImGui::CreateContext();
	mContext->io = &ImGui::GetIO();

	// Initialize ImGui with GLFW and OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window->getContext()->window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

void Showoff::GUI::deinit() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Showoff::GUI::beginDraw() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ImGui code goes here
	ImGui::Begin("Window");
	ImGui::End();
}

void Showoff::GUI::endDraw() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Showoff::Canvas::Canvas() {
	mContext = new ShowoffCanvasContext();
}

Showoff::Canvas::~Canvas() {
	delete mContext;
}

void Showoff::Canvas::init() {
	mContext->vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
}

void Showoff::Canvas::deinit() {
	// Cleanup
	nvgDeleteGL3(mContext->vg);
}

void Showoff::Canvas::beginDraw() {
	auto width = 600;
	auto height = 600;

	// Start NanoVG rendering
	nvgBeginFrame(mContext->vg, width, height, 1.0);

	// Draw a rectangle
	nvgBeginPath(mContext->vg);
	nvgRect(mContext->vg, 50, 50, 50, 50);
	nvgFillColor(mContext->vg, nvgRGBf(0.8f, 0.4f, 0.1f));
	nvgFill(mContext->vg);

	// End NanoVG rendering
	nvgEndFrame(mContext->vg);
}

void Showoff::Canvas::endDraw() {
	// End NanoVG rendering
	nvgEndFrame(mContext->vg);
}


void Showoff::init(Window* window) {
	gl.init();
	gui.init(window);
	canvas.init();
}

void Showoff::deinit() {
	canvas.deinit();
	gui.deinit();
	gl.deinit();
}

void Showoff::draw() {
	gl.beginDraw();
	canvas.beginDraw();
	gui.beginDraw();

	gl.endDraw();
	canvas.endDraw();
	gui.endDraw();
}

Window::Window(int width, int height, const char* title) {
	mContext = new WindowContext();

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

	showoff.init(this);
}

Window::~Window() {
	showoff.deinit();
	glfwDestroyWindow(mContext->window);
	delete mContext;
}


Window* Window::createWindow(int width, int height, const char* title) {
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

	return new Window(width, height, title);
}

void Window::destroyWindow(Window* window) {
	delete window;
	glfwTerminate();
}

void Window::renderLoop() {
	while (!glfwWindowShouldClose(mContext->window)) {

		showoff.draw();

		// Swap buffers and poll events
		glfwSwapBuffers(mContext->window);
		glfwPollEvents();
	}
}

WindowContext* Window::getContext() { return mContext; }
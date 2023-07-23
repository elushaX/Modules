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
//#include <nanovg.h>
//#include <nanovg_gl.h>

#include <cstdio>

class Showoff {
	class GL {
		// Showoff data
		GLuint vao{};
		GLuint vbo{};
		GLfloat vertices[9] = {
				-0.5f, -0.5f, 0.0f, // Left vertex
				0.5f, -0.5f, 0.0f,  // Right vertex
				0.0f, 0.5f, 0.0f     // Top vertex
		};

	public:
		GL() = default;

		void init() {
			// Create a Vertex Array Object (VAO)
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			// Create a Vertex Buffer Object (VBO)
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			// Set up vertex attributes
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*) nullptr);
			glEnableVertexAttribArray(0);
		}

		void deinit() {
			glDeleteBuffers(1, &vbo);
			glDeleteVertexArrays(1, &vao);
		}

		static void beginDraw() {
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		static void endDraw() {
		}
	};

	class GUI {
		ImGuiIO* io{};
		ImGuiContext* ctx{};
	public:
		GUI() = default;

		void init(GLFWwindow* window) {
			IMGUI_CHECKVERSION();
			ctx = ImGui::CreateContext();
			io = &ImGui::GetIO();

			// Initialize ImGui with GLFW and OpenGL
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 330");
		}

		void deinit() {
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		void beginDraw() {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// ImGui code goes here
			ImGui::Begin("Window");
			ImGui::End();
		}

		void endDraw() {
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
	};

	/*
	class Canvas {

		NVGcontext* vg;
		float width;
		float height;

	public:
		Canvas() = default;

		void init(int aWidth, int aHeight) {
			width = aWidth;
			height = aHeight;

			vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
		}

		void deinit() {
			// Cleanup
			nvgDeleteGL3(vg);
		}

		void beginDraw() {
			// Start NanoVG rendering
			nvgBeginFrame(vg, width, height, 1.0);

			// Clear the screen
			nvgBeginPath(vg);
			nvgRect(vg, 0, 0, width, height);
			nvgFillColor(vg, nvgRGBf(0.2f, 0.2f, 0.2f));
			nvgFill(vg);

			// Draw a rectangle
			nvgBeginPath(vg);
			nvgRect(vg, 200, 200, 300, 200);
			nvgFillColor(vg, nvgRGBf(0.8f, 0.4f, 0.1f));
			nvgFill(vg);

			// End NanoVG rendering
			nvgEndFrame(vg);
		}

		void endDraw() {
			// End NanoVG rendering
			nvgEndFrame(vg);
		}
	};
	*/

public:
	Showoff() = default;

	void init(GLFWwindow* window, int aWidth, int aHeight) {
		gui.init(window);
		gl.init();
		// canvas.init(aWidth, aHeight);
	}

	void deinit() {
		gui.deinit();
		gl.deinit();
		// canvas.deinit();
	}

	void draw() {
		gl.beginDraw();
		// canvas.beginDraw();
		gui.beginDraw();
		gui.endDraw();
		// canvas.endDraw();
		gl.endDraw();
	}

private:
	GUI gui{};
	GL gl{};
	// Canvas canvas{};
};

class Window {
	Window(int width, int height, const char* title) {
		// Create a window and OpenGL context
		window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (!window) {
			printf("Failed to create GLFW window\n");
			return;
		}

		glfwMakeContextCurrent(window);

		// Initialize GLEW
		if (glewInit() != GLEW_OK) {
			printf("Failed to initialize GLEW\n");
			return;
		}

		showoff.init(window, width, height);
	}

	~Window() {
		showoff.deinit();
		glfwDestroyWindow(window);
	}

public:

	static Window* createWindow(int width, int height, const char* title) {
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

	static void destroyWindow(Window* window) {
		delete window;
		glfwTerminate();
	}

public:
	void renderLoop() {
		while (!glfwWindowShouldClose(window)) {

			showoff.draw();

			// Swap buffers and poll events
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

private:
	GLFWwindow* window;
	Showoff showoff;
};


int main() {
	tp::ModuleManifest* deps[] = { &tp::gModuleAllocators, nullptr };
	tp::ModuleManifest testModule("Example", nullptr, nullptr, deps);

	if (!testModule.initialize()) {
		return 1;
	}

	{
		//tp::HeapAllocGlobal::startIgnore();
		auto window = Window::createWindow(800, 600, "Window 1");
		//tp::HeapAllocGlobal::stopIgnore();

		if (window) {
			window->renderLoop();
		}

		Window::destroyWindow(window);
	}

	testModule.deinitialize();
}

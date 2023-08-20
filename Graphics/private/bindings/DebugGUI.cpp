#include "Window.hpp"

#include "WindowContext.hpp"

// -------- Debug UI -------- //
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace tp {
	class Graphics::GUI::Context {
	public:
		ImGuiIO* io{};
		ImGuiContext* ctx{};
	};
}

using namespace tp;

Graphics::GUI::GUI() {
	mContext = new Context();
}

Graphics::GUI::~GUI() {
	delete mContext;
}

void Graphics::GUI::init(Window* window) {
	IMGUI_CHECKVERSION();

	mContext->ctx = ImGui::CreateContext();
	mContext->io = &ImGui::GetIO();

	mContext->io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Initialize ImGui with GLFW and OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window->getContext()->window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

void Graphics::GUI::deinit() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Graphics::GUI::proc() {
	tp::HeapAllocGlobal::startIgnore();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	tp::HeapAllocGlobal::stopIgnore();

	// ImGui code goes here
	ImGui::Begin("Window");
	ImGui::End();
}

void Graphics::GUI::draw() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
#include "Window.hpp"
#include "WindowContext.hpp"

#include "Allocators.hpp"

#include "Graphics.hpp"

// -------- Debug UI -------- //
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

namespace tp {
	class DebugGUI::Context {
	public:
		ImGuiIO* io{};
		ImGuiContext* ctx{};
	};
}

using namespace tp;

DebugGUI::DebugGUI(Window* window) {
	mContext = new Context();

	IMGUI_CHECKVERSION();
	mContext->ctx = ImGui::CreateContext();
	mContext->io = &ImGui::GetIO();

	// Initialize ImGui with GLFW and OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window->getContext()->window, true);

	// ImGui_ImplGlfw_GetBackendData();

	ImGui_ImplOpenGL3_Init("#version 330");

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("Font.ttf", 20.f);
}

DebugGUI::~DebugGUI() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	delete mContext;
}

void DebugGUI::proc() {
	tp::HeapAllocGlobal::startIgnore();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	tp::HeapAllocGlobal::stopIgnore();
}

void DebugGUI::draw() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
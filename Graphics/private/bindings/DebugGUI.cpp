#include "Window.hpp"

#include "Timing.hpp"

#include "WindowContext.hpp"

// -------- Debug UI -------- //
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "DefaultFont.bin"

namespace tp {
	class Graphics::GUI::Context {
	public:
		ImGuiIO* io{};
		ImGuiContext* ctx{};

		halnf dpmm = 3;
		halnf uiScale = 1;
		halnf fontSizeMM = 3.55f;

		Timer timer = Timer(1000);
		ualni fps {};
		ualni frames = 0;
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

	mContext->dpmm = (window->getMonitor().pixelSize.x / window->getMonitor().mmSize.x);

	setStyle();
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

	// ImGui::DockSpaceOverViewport(NULL, ImGuiDockNodeFlags_PassthruCentralNode);
}

void Graphics::GUI::draw() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	mContext->frames++;

	if (mContext->timer.isTimeout()) {
		mContext->fps = mContext->frames;
		mContext->frames = 0;
		mContext->timer.reset();
	}
}

halnf Graphics::GUI::getFontSize() const { return mContext->fontSizeMM; }
halnf Graphics::GUI::getUIScale() const { return mContext->uiScale; }

void Graphics::GUI::setDPMM(ualni dpmm) { mContext->dpmm = dpmm; }
void Graphics::GUI::setFontSize(ualni size) { mContext->fontSizeMM = size; }
void Graphics::GUI::setUIScale(ualni scale) { mContext->uiScale = scale; }

void Graphics::GUI::drawDebugInfoWindow() {
	ImGui::Begin("Debug Info");

	ImGui::Text("FPS : %llu", mContext->fps);

	ImGui::SliderFloat("UI Scale", &mContext->uiScale, 0, 10);
	ImGui::SliderFloat("Font Size MM", &mContext->fontSizeMM, 3, 20);

	setStyle();

	ImGui::End();
}

void Graphics::GUI::setStyle() {
	auto dpmm = mContext->dpmm;
  auto font_size_mm = mContext->fontSizeMM;
	auto ui_scale = mContext->uiScale;

	#define VAL(val) (val * dpmm / 3 * ui_scale)
	#define VEC(x, y) ImVec2(x * dpmm / 3, y * dpmm / 3 * ui_scale)

	ImGuiStyle* style = &ImGui::GetStyle();
	auto& io = *mContext->io;
	bool first_init = io.Fonts->Fonts.Size == 0;
	auto font_path = ".";
	//auto font_path = getFontPath();

	// supports PDMM from 0.3 to 10
	const float min_dpmm = 0.3f;
	const float max_dpmm = 20.f;
	dpmm = tp::clamp(dpmm, min_dpmm, max_dpmm);

	// calc font
	const float font_size_mm_max = 10;
	const float font_size_pixels_min = 9;
	float font_size_mm_min = font_size_pixels_min / dpmm;
	const int font_quality_steps = 3;
	font_size_mm = tp::clamp(font_size_mm, font_size_mm_min, font_size_mm_max);

	float font_sizes[font_quality_steps];
	for (auto i = 0; i < font_quality_steps; i++) {
		auto mm = font_size_mm_min + (font_size_mm_max - font_size_mm_min) * tp::pow(((tp::halnf) (i + 1) / font_quality_steps), 3);
		font_sizes[i] = dpmm * mm;
	}

	if (first_init) {
		for (auto i = 0; i < font_quality_steps; i++) {
			//io.Fonts->AddFontFromFileTTF(font_path, font_sizes[i] * 1);
			ImFontConfig font_cfg_template;
			font_cfg_template.FontDataOwnedByAtlas = false;
			io.Fonts->AddFontFromMemoryTTF(CONSOLAB_TTF, CONSOLAB_TTF_len, font_sizes[i] * 1, &font_cfg_template);
		}
	}

	// select fonts
	auto const pixels_required = dpmm * font_size_mm;
	auto idx = -1;
	for (auto i = 0; i < font_quality_steps; i++) {
		idx = i;
		if (pixels_required <= font_sizes[i]) {
			break;
		}
	}

	io.FontDefault = io.Fonts->Fonts[idx];
	io.FontGlobalScale = font_size_mm / (font_size_mm_min + ((font_size_mm_max - font_size_mm_min) / font_quality_steps) * (idx + 1));
	io.FontGlobalScale = pixels_required / font_sizes[idx];

	auto rounding = VAL(5);
	auto pudding = VEC(7, 7);

	style->WindowPadding = pudding;
	style->WindowRounding = rounding;
	//style->WindowMinSize = VEC(11, 11);
	style->ChildRounding = rounding;
	style->PopupRounding = rounding;
	style->FramePadding = pudding;
	style->FrameRounding = rounding;
	style->ItemSpacing = VEC(4, 11);
	style->ItemInnerSpacing = VEC(9, 4);
	style->CellPadding = pudding;
	style->TouchExtraPadding = pudding;
	style->IndentSpacing = VAL(25);
	style->ColumnsMinSpacing = VAL(5);
	style->ScrollbarSize = VAL(16);
	style->ScrollbarRounding = rounding;
	style->GrabMinSize = VAL(2);
	style->GrabRounding = rounding;
	style->LogSliderDeadzone = VAL(2);
	style->TabRounding = rounding;
	style->TabMinWidthForCloseButton = VAL(5);
	style->DisplayWindowPadding = pudding;
	style->DisplaySafeAreaPadding = pudding;
	style->MouseCursorScale = VAL(5);

	style->FrameBorderSize = VAL(0);
	style->WindowBorderSize = VAL(1.5f);
	style->ChildBorderSize = VAL(2);

	style->WindowTitleAlign = VEC(0.5f, 0.6f);
	style->WindowMenuButtonPosition = ImGuiDir_Right;

	if (!first_init)
		return;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.27f, 0.33f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.43f, 0.43f, 0.53f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.09f, 0.09f, 0.10f, 0.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15f, 0.15f, 0.19f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.21f, 0.20f, 0.25f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.29f, 0.35f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.48f, 0.50f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.53f, 0.57f, 0.64f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.69f, 0.74f, 0.83f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.27f, 0.33f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.43f, 0.43f, 0.53f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.27f, 0.27f, 0.33f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.33f, 0.33f, 0.40f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.37f, 0.37f, 0.40f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.33f, 0.35f, 0.38f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.37f, 0.39f, 0.42f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.16f, 0.17f, 0.19f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.33f, 0.35f, 0.38f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.28f, 0.34f, 0.42f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.23f, 0.23f, 0.28f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.28f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.43f, 0.45f, 0.48f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.81f, 0.81f, 0.81f, 0.18f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.33f, 0.35f, 0.38f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.49f, 0.51f, 0.54f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.13f, 0.13f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.23f, 0.20f, 0.20f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.38f, 0.22f, 0.22f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.64f, 0.75f, 0.83f, 0.18f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.39f, 0.39f, 0.47f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.27f, 0.27f, 0.33f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.24f, 0.24f, 0.29f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.65f, 0.65f, 0.65f, 0.70f);

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}
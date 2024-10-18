#include "WidgetApplication.hpp"

#include "imgui.h"

using namespace tp;

void WidgetApplication::debugUI() {
	ImGui::Text("Proc ms : %i", (int) mProcTime);
	ImGui::SameLine(); ImGui::Text("Draw ms : %i", (int) mDrawTime);
}


#include "GUI.h"

#include "Rect.hpp"

#include "interpreter/Interpreter.hpp"

#include "imgui.h"
#include "imgui_internal.h"

using namespace tp;
using namespace obj;

namespace ImGui {

	bool SubMenuBegin(const char* desc, int level);
	void SubMenuEnd(int level);

	void ToolTip(const char* desc);

	struct PopupData {
		bool opened = false;
		bool ishovered = false;
		tp::Vec2F p1;
		tp::Vec2F p2;
		operator bool() { return opened; }
	};

	PopupData HoverPopupBegin(const char* id, tp::Vec2F size, tp::Vec2F pos = tp::Vec2F(-1), int flags = 0);
	void HoverPopupEnd(PopupData& in);

	void ApplyStyle(tp::halnf dpmm, float font_size_mm = 5, float ui_scale = 1);
};

ImGui::PopupData
ImGui::HoverPopupBegin(const char* str_id, tp::Vec2F size, tp::Vec2F pos_p, ImGuiPopupFlags popup_flags) {
	ImGui::PopupData out;
	out.ishovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

	if (out.ishovered) {
		ImVec2 pos;

		if (pos_p == tp::Vec2F(-1)) {
			pos = GImGui->CurrentWindow->DC.CursorPos;
		} else {
			pos.x = pos_p.x;
			pos.y = pos_p.y;
		}

		ImGui::SetNextWindowPos(pos);
		out.p1 = { pos.x, pos.y };

		ImGui::OpenPopup(str_id);

		out.p2 = out.p1;
		out.p2.x += ImGui::GetWindowWidth();
	}

	if (BeginPopup(str_id, ImGuiWindowFlags_NoMove)) {
		out.opened = true;

		auto pos = GetWindowPos();
		out.p1 = { pos.x, pos.y };
		out.p2 = out.p1;
		out.p2.x += ImGui::GetWindowWidth();
	}
	return out;
}

void ImGui::HoverPopupEnd(ImGui::PopupData& in) {

	if (!in.opened) {
		return;
	}

	in.ishovered |= IsWindowHovered(
		ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
		ImGuiHoveredFlags_ChildWindows
	);

	tp::Vec2F mousepos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
	tp::halnf tollerance = 10;
	tp::RectF tollerace_rect(tp::Vec2F(in.p1.x, in.p1.y - tollerance), tp::Vec2F(in.p2.x - in.p1.x, tollerance * 2.f));
	bool is_tollerance = tollerace_rect.isInside(mousepos);

	if (!(in.ishovered || is_tollerance)) {
		CloseCurrentPopup();
	}

	EndPopup();
}

obj::ObjectsGUI::~ObjectsGUI() {
	if (mClipboard) {
		obj::objects_api::destroy(mClipboard);
	}
	for (auto iter : mViewStack) {
		obj::objects_api::destroy(iter->obj);
	}
}

void obj::ObjectsGUI::setClipboard(obj::Object* obj) {
	if (mClipboard) {
		obj::objects_api::destroy(mClipboard);
	}

	mClipboard = obj;

	if (mClipboard) {
		obj::objects_api::increaseReferenceCount(obj);
	}
}

obj::Object* obj::ObjectsGUI::getClipboard() { return mClipboard; }

obj::Object* imgui_object_create_menu(obj::TypeGroups* type_group = nullptr) {

	obj::Object* newo = nullptr;

	if (!type_group) {
		type_group = &obj::gObjectsContext->type_groups;
	}

	for (auto childo : *type_group->getChilds()) {

		if (childo->val->isGroup()) {
			if (ImGui::BeginMenu((childo->key).c_str())) {
				newo = imgui_object_create_menu(childo->val);
				ImGui::EndMenu();
			}
			continue;
		}

		if (ImGui::Button((childo->key).c_str(), { 100, 0 })) {
			if (childo->key == "null") {
				newo = objects_api::getNull();
			} else {
				newo = objects_api::createByName(childo->key.c_str());
			}
		}
	}

	return newo;
}

obj::ObjectsGUI::ObjectsGUI() { assert(gObjectsContext && "Objects library is not initialized"); }

void obj::ObjectsGUI::cd(obj::Object* child, const std::string& name) {
	mActive = child;
	mViewStack.pushBack({ mActive, name });
	obj::objects_api::increaseReferenceCount(child);
}

void obj::ObjectsGUI::cdup() {
	if (mViewStack.length() > 1) {
		obj::objects_api::destroy(mViewStack.lastNode()->data.obj);

		mViewStack.popBack();
		mActive = mViewStack.lastNode()->data.obj;
	}
}

void obj::ObjectsGUI::preview(obj::Object* obj) {
	if (objects_api::cast<LinkObject>(obj)) {
		auto linko = objects_api::cast<LinkObject>(obj);

		bool no_link_preview = !linko->getLink();
		tp::alni max_depth = 5;
		tp::alni depth_count = 0;
		tp::Map<tp::alni, tp::alni> link_lookup;
		obj::LinkObject* link_iter = linko;
		while (link_iter && link_iter->getLink() && !no_link_preview) {
			link_iter = objects_api::cast<LinkObject>(link_iter->getLink());
			depth_count++;

			if (link_lookup.presents(tp::alni(link_iter))) {
				no_link_preview = true;
			} else {
				link_lookup.put(tp::alni(link_iter), 0);
			}
			no_link_preview |= max_depth < depth_count;
		}

		if (linko->getLink()) {
			if (!no_link_preview) {
				ImGui::Text("=> ");
				ImGui::SameLine();
				preview(linko->getLink());
			} else {
				ImGui::Text("Max preview depth exceeded");
			}
		} else {
			ImGui::Text("Link Is Null");
		}
	} else if (objects_api::cast<IntObject>(obj)) {
		tp::alni& val = objects_api::cast<IntObject>(obj)->val;
		int gui_val = int(val);
		ImGui::InputInt(" ", &gui_val);
		val = tp::alni(gui_val);
	} else if (objects_api::cast<StringObject>(obj)) {
		auto stringo = objects_api::cast<StringObject>(obj);
		static char val[2048] = { " " };
		if (stringo->val != val) {
			assert(stringo->val.size() < 2048);
			tp::memCopy(val, stringo->val.c_str(), stringo->val.size() + 1);
		}
		ImGui::InputText("", val, 2048);
		if (stringo->val != val) {
			stringo->val = val;
		}
	} else if (objects_api::cast<BoolObject>(obj)) {
		boolView((obj::BoolObject*) obj);
	} else if (objects_api::cast<FloatObject>(obj)) {
		tp::alnf& val = objects_api::cast<FloatObject>(obj)->val;
		auto gui_val = float(val);
		ImGui::InputFloat(" ", &gui_val);
		val = tp::alnf(gui_val);

	} else if (objects_api::cast<EnumObject>(obj)) {
		enumView((obj::EnumObject*) obj);
	} else if (objects_api::cast<ColorObject>(obj)) {
		colorView((obj::ColorObject*) obj);
	} else {
		ImGui::Text(obj->type->name);
		ImGui::SameLine();
		ImGui::Text(" (No Preview) ");
	}
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::interpreterView(obj::InterpreterObject* self) {
	using namespace ImGui;

	auto& interp = self->mInterpreter;

	// not running state
	if (interp.finished()) {
		Text("State: Not Runnig");

		if (Selectable("Debug")) {
			self->debug();
		}

		auto res = ObjectsGUI::classView(self);
		if (res) {
			return res;
		}

		return {};
	}

	// running state
	BeginChild("dockable");
	{
		auto id = ImGui::GetCurrentWindow()->ID;
		DockSpace(id);
	}
	EndChild();

	Begin("ByteCodeView");
	{

		if (Button("Step")) {
			interp.stepBytecode();
			if (interp.finished()) {
				End();
				return {};
			}
		}

		SameLine();
		if (Button("Step In")) {
			interp.stepBytecodeIn();
			if (interp.finished()) {
				End();
				return {};
			}
		}

		SameLine();
		if (Button("Step out")) {
			interp.stepBytecodeOut();
			if (interp.finished()) {
				End();
				return {};
			}
		}

		BeginChild("frame", { 0, 0 }, 0, ImGuiWindowFlags_NoBackground);

		const auto bytecode = interp.mCallStack.getBytecode();
		tp::halni idx = 0;

		for (auto ip = 0; ip < bytecode->mInstructions.size();) {
			auto opcode = bytecode->mInstructions[ip];
			auto info = obj::gOpcodeInfos.fetch(opcode);
			PushID(info.name);

			TextColored(ImVec4(1.f, 1.f, 1.f, 0.5f), "%i", idx);
			SameLine();
			if (bytecode->mInstructionIdx == ip) {
				TextColored(ImVec4(1.f, 0.5f, 0.5f, 1.f), info.name);
			} else {
				Text(info.name);
			}

			// INFO
			auto hd = HoverPopupBegin(std::to_string((tp::alni) ip).c_str(), { 400, 250 });
			if (hd) {
				Text(info.desc);
				if (info.operands.len) {
					Text("Operands :");
					for (auto i = 0; i < info.operands.len; i++) {
						auto op = info.operands.buff[i];
						Text("  %i : %s : %s", i, op.obj_type, op.desc);
					}
				}
				if (info.params.len) {
					Text("Params : ");
					for (auto i = 0; i < info.params.len; i++) {
						auto param = info.params.buff[i];
						Text("  %i : %i bytes : %s ", i, param.bytes, param.desc);
					}
				}
			}
			HoverPopupEnd(hd);

			PopID();
			ip += info.opsize();
			idx++;
		}

		EndChild();
	}
	End();

	Begin("SourceCodeView");
	{ stringView(interp.mCallStack.mStack.last().mMethod->mBytecodeLink->mReadable); }
	End();

	Begin("OperandStack");
	{
		if (!interp.mOperandsStack.mIdx) {
			Text("No Operands");
		}
		for (auto i = 0; i < interp.mOperandsStack.mIdx; i++) {
			auto operand = interp.mOperandsStack.mBuff[i];
			tp::alni operand_val = (tp::alni) operand;

			if (operand_val == 0) {
				Text("nullptr");
			} else if (tp::uint2(operand_val) == 0) {
				Text("2 BYTE VAL : %i", tp::uint2(operand_val));
			} else {
				if (Selectable(operand->type->name, false, 0, ImVec2(100, 0))) {
					End();
					return { operand, "operand" };
				}
				SameLine();

				PushID((tp::alni) operand);
				preview(operand);
				PopID();
			}
		}
	}
	End();

	Begin("ScopeStack");
	{
		for (auto i = 0; i < interp.mScopeStack.mIdx; i++) {
			if (TreeNode(std::to_string((tp::alni) i).c_str())) {

				auto& scope = interp.mScopeStack.mBuff[i];

				if (TreeNode("Locals")) {
					for (auto local : scope.mLocals) {
						if (Selectable(local->key.c_str(), false, 0, ImVec2(100, 0))) {
							TreePop();
							TreePop();
							End();
							return { local->val, local->key };
						}

						PushID(local->key.c_str());
						SameLine();
						preview(local->val);
						PopID();
					}
					TreePop();
				}

				if (TreeNode("Temps")) {
					tp::alni idx = 0;
					for (auto tmp : scope.mTemps) {
						if (Selectable(std::to_string(idx).c_str(), false, 0, ImVec2(100, 0))) {
							TreePop();
							TreePop();
							End();
							return { tmp.data(), "temp" };
						}
						idx++;
					}
					TreePop();
				}
				TreePop();
			}
		}
	}
	End();

	Begin("CallStack");
	{
		for (auto i = 0; i < interp.mCallStack.mStack.size(); i++) {
			auto method = interp.mCallStack.mStack[i].mMethod;
			PushID(i);
			if (Selectable("method")) {
				PopID();
				End();
				return { method, "call stack method" };
			}
			PopID();
		}
	}
	End();

	Begin("ConstPool");
	{
		auto bc = interp.mCallStack.getBytecode();
		tp::alni idx = 0;
		for (auto const_obj : bc->mConstants) {
			PushID(idx);
			if (Button(const_obj.data()->type->name)) {
				End();
				PopID();
				return { const_obj.data(), std::to_string(idx) };
			}
			SameLine();
			preview(const_obj.data());
			PopID();
			idx++;
		}
	}
	End();

	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::colorView(obj::ColorObject* in) {
	ImGui::ColorEdit4(" ", &in->mCol.r);
	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::enumView(obj::EnumObject* obj) {

	if (!obj->entries) {
		ImGui::Text("enum is uninitialized");
		return {};
	}

	if (ImGui::BeginCombo(" ", obj->getActiveName())) {

		for (tp::uhalni idx = 0; idx < obj->nentries; idx++) {
			if (ImGui::Selectable(obj->getItemName(idx))) {
				obj::objects_api::set(obj, tp::alni(idx));
			}
		}

		ImGui::EndCombo();
	}

	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::boolView(obj::BoolObject* obj) {
	tp::alni& val = obj->val;
	bool gui_val = bool(val);
	ImGui::Checkbox(gui_val ? "True" : "False", &gui_val);
	val = tp::alni(gui_val);
	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::floatView(obj::FloatObject* obj) {
	tp::alnf& val = obj->val;
	float gui_val = float(val);
	ImGui::InputFloat("Value", &gui_val);
	val = tp::alnf(gui_val);
	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::nullView(obj::NullObject* in) {
	ImGui::Text("Null Object.");
	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::linkoView(obj::LinkObject* obj) {

	if (obj->getLink()) {

		ImGui::Text("%s at %x", obj->getLink()->type->name, obj->getLink());

		if (ImGui::Selectable("View")) {
			return { obj->getLink(), "adress" };
		}
	} else {
		ImGui::Text("Link Is Null");
	}

	if (ImGui::Selectable("Set from clipboard")) {
		obj->setLink(mClipboard);
	}

	if (ImGui::Selectable("Copy value")) {
		setClipboard(obj->getLink());
	}
	if (ImGui::Selectable("Set Null")) {
		obj->setLink(nullptr);
	}

	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::intoView(obj::IntObject* obj) {
	ImGui::Text("Int Value: ");
	ImGui::SameLine();
	int val = (int) obj->val;
	ImGui::InputInt(" ", &val);
	obj->val = (tp::alni) val;
	return {};
}

void obj::ObjectsGUI::dictViewDrawCreate(obj::DictObject* dict) {
	if (ImGui::BeginPopupContextItem("child_2", ImGuiPopupFlags_MouseButtonRight)) {

		if (mClipboard) {
			if (ImGui::Selectable("Paste")) {
				tp::alni idx = 1;
				auto name_base = std::string("clipboard ") + std::string(mClipboard->type->name) + " ";
				auto name_out = name_base;

				while (dict->presents(name_out)) {
					name_out = name_base + std::to_string(idx);
					idx++;
				}

				dict->put(name_out, mClipboard);
			}
		}

		if (ImGui::BeginMenu("Create")) {

			obj::Object* newo = imgui_object_create_menu();

			if (newo) {
				tp::alni idx = 1;
				auto name_base = std::string("new ") + std::string(newo->type->name) + " ";
				auto name_out = name_base;

				while (dict->presents(name_out)) {
					name_out = name_base + std::to_string(idx);
					idx++;
				}

				dict->put(name_out, newo);
			}

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
}

void obj::ObjectsGUI::dictViewEdit(obj::DictObject* dict, const std::string& key, obj::Object* obj, bool& popup) {
	if (ImGui::BeginPopupContextItem(key.c_str(), ImGuiPopupFlags_MouseButtonRight)) {
		popup = true;

		ImGui::Text("%s at %x", obj->type->name, obj);

		if (key.size() > 100) {
			ImGui::Text("name is too large");
		} else {

			if (mLastNameEditObject != obj) {
				tp::memCopy(mNameEdit, key.c_str(), key.size() + 1);
				mLastNameEditObject = obj;
			}

			if (ImGui::InputTextEx(" ", "new name", mNameEdit, 100, { 140, 30 }, ImGuiInputTextFlags_EnterReturnsTrue)) {
				auto idx = dict->presents(mNameEdit);
				if (bool(idx)) {
					// Notify("Object with such name Already Exists");
				} else {
					obj::objects_api::increaseReferenceCount(obj);
					dict->remove(key);
					auto id = std::string(mNameEdit);
					dict->put(id, obj);
					obj::objects_api::destroy(obj);
				}
			}
		}

		if (ImGui::Selectable("Remove")) {
			dict->remove(key);
		}

		if (ImGui::Selectable("Copy Link")) {
			setClipboard(obj);
		}

		ImGui::EndPopup();
	}
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::dictView(obj::DictObject* obj) {
	bool popup = false;
	auto table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

	ImGui::BeginChild("frame", { 0, 0 }, 0, ImGuiWindowFlags_NoBackground);
	{
		if (!obj->getItems().size()) {
			ImGui::Text("Dictinary Is Empty. ");
		}

		if (ImGui::BeginTable("Members", 2, table_flags)) {
			ImGui::TableSetupColumn("name");
			ImGui::TableSetupColumn("info");
			for (auto childo : obj->getItems()) {

				ImGui::TableNextRow(0, 30);
				ImGui::TableSetColumnIndex(0);

				if (ImGui::Selectable(childo->key.c_str())) {
					ImGui::EndTable();
					ImGui::EndChild();
					return { childo->val, { std::string(childo->val->type->name) + " " + childo->key } };
				}

				dictViewEdit(obj, childo->key, childo->val, popup);

				ImGui::TableSetColumnIndex(1);
				ImGui::PushID((int) childo->key.c_str()[0]);
				preview(childo->val);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}
	ImGui::EndChild();

	if (!popup) {
		dictViewDrawCreate(obj);
	}

	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::listView(obj::ListObject* obj) {
	bool popup = false;
	ViewStackNode out;

	if (!obj->getItems().length()) {
		ImGui::Text("List Is Empty. ");
	}

	ImGui::BeginChild("frame", { 0, 0 }, 0, ImGuiWindowFlags_NoBackground);

	if (ImGui::BeginTable("Items", 2)) {
		ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("info");

		tp::alni idx = 0;
		for (auto childo : obj->getItems()) {
			ImGui::TableNextRow(0, 36);

			ImGui::TableSetColumnIndex(0);
			if (ImGui::Selectable(std::to_string(idx).c_str())) {
				out = ViewStackNode(childo.data(), { std::string(childo->type->name) + "at" + std::to_string(idx) });
				break;
			}

			{
				if (ImGui::BeginPopupContextItem(std::to_string(idx).c_str(), ImGuiPopupFlags_MouseButtonRight)) {
					popup = true;

					ImGui::Text("%s at %x", childo->type->name, childo.data());

					if (ImGui::Selectable("Remove")) {
						obj->delNode(childo.node());
						ImGui::EndPopup();
						break;
					}

					if (ImGui::Selectable("Copy Link")) {
						setClipboard(childo.data());
						ImGui::EndPopup();
						break;
					}

					if (childo.node()->prev && ImGui::Selectable("Move Up")) {
						tp::swapV(childo.node()->prev->data, childo.data());
						ImGui::EndPopup();
						break;
					}

					if (childo.node()->next && ImGui::Selectable("Move Down")) {
						tp::swapV(childo.node()->next->data, childo.data());
						ImGui::EndPopup();
						break;
					}

					ImGui::EndPopup();
				}
			}

			ImGui::TableSetColumnIndex(1);

			ImGui::PushID((int) idx);
			preview(childo.data());
			ImGui::PopID();

			idx++;
		}
		ImGui::EndTable();
	}

	ImGui::EndChild();

	if (!popup && ImGui::BeginPopupContextItem("child_2", ImGuiPopupFlags_MouseButtonRight)) {

		if (ImGui::Selectable("Paste")) {
			obj->pushBack(mClipboard);
		}

		if (ImGui::BeginMenu("Create")) {

			obj::Object* newo = imgui_object_create_menu();
			if (newo) {
				obj->pushBack(newo);
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	return out;
}

void drawStrView(std::string& in) {
	static char val[2048] = { " " };

	if (in != val) {
		assert(in.size() < 2048);
		tp::memCopy(val, in.c_str(), in.size() + 1);
	}

	ImGui::BeginChild("str_edit");
	ImGui::InputTextMultiline(" ", val, 2048, { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() });
	ImGui::EndChild();

	if (in != val) {
		in = val;
	}
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::stringView(obj::StringObject* in) {
	drawStrView(in->val);
	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::classView(obj::ClassObject* self) {

	bool popup = false;
	auto dict = self->members;

	ImGui::BeginChild("frame", { 0, 0 }, 0, ImGuiWindowFlags_NoBackground);

	if (!dict->getItems().size()) {
		ImGui::Text("No Members");
	}

	auto table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;
	auto tree_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed;
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	auto n_methods = 0;
	for (auto childo : dict->getItems()) {
		if (childo->val->type == &obj::MethodObject::TypeData) {
			n_methods++;
		}
	}

	if (n_methods != dict->getItems().size() && ImGui::TreeNodeBehavior(window->GetID("Members"), tree_flags, "Members", 0)) {
		if (ImGui::BeginTable("Members", 2, table_flags)) {
			ImGui::TableSetupColumn("name");
			ImGui::TableSetupColumn("info");

			tp::alni idx = 0;
			for (auto childo : dict->getItems()) {
				if (childo->val->type == &obj::MethodObject::TypeData) {
					continue;
				}

				ImGui::TableNextRow(0, 30);
				ImGui::TableSetColumnIndex(0);

				if (ImGui::Selectable(childo->key.c_str())) {
					ImGui::EndTable();
					ImGui::TreePop();
					ImGui::EndChild();
					return { childo->val, { std::string(childo->val->type->name) + childo->key } };
				}

				dictViewEdit(dict, childo->key, childo->val, popup);

				ImGui::TableSetColumnIndex(1);
				ImGui::PushID((int) idx);
				preview(childo->val);
				ImGui::PopID();
			}
			ImGui::EndTable();

			idx++;
		}
		ImGui::TreePop();
	}

	if (n_methods && ImGui::TreeNodeBehavior(window->GetID("Methods"), tree_flags, "Methods", 0)) {
		for (auto childo : dict->getItems()) {
			if (childo->val->type == &obj::MethodObject::TypeData) {
				if (ImGui::Selectable(childo->key.c_str())) {
					ImGui::TreePop();
					ImGui::EndChild();
					return { childo->val, { std::string(childo->val->type->name) + childo->key } };
				}
				dictViewEdit(dict, childo->key, childo->val, popup);
			}
		}
		ImGui::TreePop();
	}

	ImGui::EndChild();

	if (!popup) {
		dictViewDrawCreate(dict);
	}

	return {};
}

obj::ObjectsGUI::ViewStackNode obj::ObjectsGUI::methodView(obj::MethodObject* in) {

	if (in->mBytecodeLink->mBytecode.mInstructions.size()) {
		if (ImGui::Button("Recompile")) {
			in->compile();
		}

		ImGui::SameLine();
		if (ImGui::Button("Execute")) {
			obj::Interpreter interp;
			interp.execAll(in);
		}

	} else {
		if (ImGui::Button("Compile")) {
			in->compile();
		}
	}

	ImGui::SameLine();
	auto ret = stringView(in->mBytecodeLink->mReadable);

	if (ret.obj) {
		return ret;
	}

	return {};
}

void obj::ObjectsGUI::explorer() {

	if (ImGui::Button("<")) {
		cdup();
	}

	ImGui::SameLine();
	ImGui::BeginChild(
		"child_path",
		{ 0, 45 },
		false,
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_HorizontalScrollbar
	);
	tp::List<ViewStackNode*> rev_path;
	for (auto childo = mViewStack.lastNode(); childo; childo = childo->prev) {
		rev_path.pushBack(&childo->data);
	}

	tp::alni idx = 0;
	for (auto childo = rev_path.lastNode(); childo; childo = childo->prev) {
		ImGui::PushID((int) idx);
		bool go_back = false;
		if (childo == rev_path.lastNode()) {
			go_back = ImGui::Button(childo->data->id.c_str());
			ImGui::SameLine();
		} else {
			go_back = ImGui::Button((childo->data->id).c_str());
			ImGui::SameLine();
		}
		ImGui::PopID();

		if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight)) {

			obj::Object* curretn_object = childo->data->obj;
			ImGui::Text("%s at %x", curretn_object->type->name, curretn_object);
			ImGui::Separator();

			if (ImGui::Selectable("Copy Link")) {
				setClipboard(curretn_object);
			}

			if (ImGui::Selectable("Instantiate  ")) {
				setClipboard(obj::objects_api::instantiate(curretn_object));
				// Notify("Object copied to clipboard");
			}

			ImGui::Separator();

			static char path_str[100] = { "data.o\0" };
			static bool compressed = true;
			ImGui::InputTextEx(" ", "save path", path_str, 100, { 100, 30 }, 0);

			bool save_object = ImGui::Button("Save Object");
			ImGui::SameLine();
			ImGui::Checkbox("Compressed", &compressed);
			bool load_object = ImGui::Button("Load Object");

			if (save_object) {
				obj::objects_api::save(curretn_object, path_str, compressed);
				// Notify("Object saved");
			}

			if (load_object) {
				obj::Object* loadedo = obj::objects_api::load(path_str);
				if (loadedo) {
					setClipboard(loadedo);
					// Notify("Object copied to clipboard");
				} else {
					// Notify("Can't load Object");
				}
			}

			ImGui::EndPopup();
		}

		if (go_back) {
			while (&mViewStack.lastNode()->data != childo->data) {
				cdup();
			}
			mActive = childo->data->obj;
			ImGui::EndChild();
			return;
		}
		idx++;
	}
	ImGui::Text(" ");
	ImGui::EndChild();

	ImGui::Separator();

	ViewStackNode new_active;
	if (objects_api::cast<NullObject>(mActive)) {
		new_active = nullView((obj::NullObject*) mActive);
	} else if (objects_api::cast<LinkObject>(mActive)) {
		new_active = linkoView((obj::LinkObject*) mActive);
	} else if (objects_api::cast<IntObject>(mActive)) {
		new_active = intoView((obj::IntObject*) mActive);
	} else if (objects_api::cast<StringObject>(mActive)) {
		new_active = stringView((obj::StringObject*) mActive);
	} else if (objects_api::cast<ListObject>(mActive)) {
		new_active = listView((obj::ListObject*) mActive);
	} else if (objects_api::cast<DictObject>(mActive)) {
		new_active = dictView((obj::DictObject*) mActive);
	} else if (objects_api::cast<InterpreterObject>(mActive)) {
		new_active = interpreterView((obj::InterpreterObject*) mActive);
	} else if (objects_api::cast<ClassObject>(mActive)) {
		new_active = classView((obj::ClassObject*) mActive);
	} else if (objects_api::cast<BoolObject>(mActive)) {
		new_active = boolView((obj::BoolObject*) mActive);
	} else if (objects_api::cast<FloatObject>(mActive)) {
		new_active = floatView((obj::FloatObject*) mActive);
	} else if (objects_api::cast<EnumObject>(mActive)) {
		new_active = enumView((obj::EnumObject*) mActive);
	} else if (objects_api::cast<MethodObject>(mActive)) {
		new_active = methodView((obj::MethodObject*) mActive);
	} else if (objects_api::cast<ColorObject>(mActive)) {
		new_active = colorView((obj::ColorObject*) mActive);
	} else {
		ImGui::Text("Preview is Unavailable");
	}

	if (new_active != 0) {
		cd(new_active.obj, new_active.id);
	}
}

void obj::ObjectsGUI::properties(const obj::ObjectType* type, bool top_of_tree_vew) {

	assert(type);
	assert(mActive);
	if (mActive->type != type) return;

	ImGui::Text(" RefCount : %i", obj::objects_api::getReferenceCount(mActive));

	ImGui::Text("  Type : %s", type->name);

	if (ImGui::TreeNode("Description ")) {
		if (type->description) {
			ImGui::Text(type->description);
		} else {
			ImGui::Text("Description is not given");
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Size")) {
		if (0 && ImGui::TreeNode("Top Level Type")) {

			ImGui::Text("RAM : ");
			ImGui::Indent();
			ImGui::Text("Only Structure : %i bytes", type->size - (type->base ? type->base->size : 0));
			ImGui::Text("With Non-Object Links : Not Supported");
			ImGui::Unindent();

			ImGui::Text("Disk :");
			ImGui::Indent();
			ImGui::Text("Only Structure : Currently Not Supported");
			ImGui::Text("With Object Links : Not Supported");
			ImGui::Unindent();

			ImGui::TreePop();
		}
		// if (ImGui::TreeNode("Full Type Hierarchy")) {

		ImGui::Text("RAM : ");
		ImGui::Indent();
		ImGui::Text("Only Structure : %i bytes", type->size);
		ImGui::Text("With Non-Object Links : %i bytes", obj::objects_api::objsize_ram(mActive));
		ImGui::Text("Full Size Recursive : %i bytes", obj::objects_api::objsize_ram_recursive(mActive));
		ImGui::Unindent();

		ImGui::Text("Disk :");
		ImGui::Indent();
		ImGui::Text("Only Structure : %i bytes", obj::objects_api::objsize_file(mActive));
		ImGui::Text("With Object Links : %i bytes", obj::objects_api::objsize_file_recursive(mActive));
		ImGui::Unindent();

		// ImGui::TreePop();
		//}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Base Type")) {
		if (type->base) {
			properties(type->base, false);
		} else {
			ImGui::Text("No base type");
		}
		ImGui::TreePop();
	}
}

void obj::ObjectsGUI::draw() {
	if (!mShowDebugInfo) {
		return;
	}

	if (mViewStack.length()) {
		if (ImGui::Begin("Explorer")) {
			explorer();
		}
		ImGui::End();

		if (ImGui::Begin("Objet Info")) {
			properties(mActive->type);
		}
		ImGui::End();

	} else {
		ImGui::Text("ViewStack is Empty");
	}

	auto& io = ImGui::GetIO();
	mCaptureInput = (io.WantCaptureMouse || io.WantCaptureKeyboard);
}
 /*

#include "app.h"

#include "glutils.h"

#include "shader.h"
#include "strings.h"

#include "imgui_internal.h"

#include "ImGuiUtils.h"

using namespace tp;

void StrokeApp::draw_explorer() {
	using namespace ImGui;
	//objects_gui.oexplorer(200);
	//if (SubMenuBegin("Type Info", 1)) {
		//objects_gui.oproperties(objects_gui.active->type);
		//SubMenuEnd(1);
	//}
}

void StrokeApp::draw_brush_properties(rectf rect) {
	using namespace ImGui;
	if (ImGui::Button(project->sampler.eraser ? "Eraser" : "Pencil")) {
		project->sampler.eraser = !project->sampler.eraser;
	} ImGui::SameLine();

	SetNextItemWidth(rect.z / 2.f);
	//ImGui::SetNextItemWidth(rect.w);
	if (project->sampler.eraser) {
		ImGui::SliderFloat(" ", &project->sampler.eraser_size, 0.001f, 0.2f);
	} else {
		ImGui::SliderFloat(" ", &project->sampler.screen_thikness, 0.001f, 0.2f);

		SetNextItemWidth(rect.z / 1.5f);
		int flags = ImGuiColorEditFlags_NoLabel |
			ImGuiColorEditFlags_NoSmallPreview |
			ImGuiColorEditFlags_NoSidePreview |
			ImGuiColorEditFlags_NoInputs;
		ImGui::ColorPicker4("Color", &project->sampler.stroke_col.r, flags);

		PushItemWidth(rect.z / 4.f);
		if (SubMenuBegin("Sampler Settings", 1)) {

			if (SubMenuBegin("Denoising Passes", 2)) {
				SliderInt("Position", &project->denoise_passes, 0, 10);
				SliderInt("Thickness", &project->denoise_passes_thikness, 0, 10);
				SubMenuEnd(2);
			}

			if (SubMenuBegin("Size Reduction", 2)) {
				SliderFloat("Input Precision", &project->sampler.screen_precision, 0, 0.005);
				Checkbox("Auto Apply", &project->auto_reduction);
				SubMenuEnd(2);
			}

			if (SubMenuBegin("Pen Pressure", 2)) {
				//ImGui::Bezier("easeOutSine", tablet_input_formater);
				SubMenuEnd(2);
			}

			SubMenuEnd(1);
		}
		PopItemWidth();
	}


}

void StrokeApp::draw_toolbar(rectf rec) {
	const halnf nbuttons = project ? 6 : 1;
	halnf buttsize = clamp(rec.z / nbuttons, 5.f, 50.f) + 30;
	vec2f buttrec = {buttsize - 20, buttsize - 50};

	vec2f midpoint = (rec.pos + rec.size_vecw()) / 2.f;
	rec.size = {buttsize * nbuttons + 20, buttsize - 30};
	rec.pos = midpoint - (rec.size / 2.f);

	//ImGui::SetNextWindowPos(ImVec2(rec.x, window.size.y - rec.y));
	ImGui::SetNextWindowSize(ImVec2(rec.z, rec.w));
	ImGui::Begin("ToolBar", 0, ImGui::frame_window);

	vec2f popup_size = vec2f(200, 300);

	auto popup = ImGui::ButtonHoverPopupBegin("Explore", buttrec, popup_size);
	if (popup) { draw_explorer(); }
	ImGui::HoverPopupEnd(popup);

	if (!project) {
		ImGui::End();
		return;
	}

	ImGui::SameLine();
	popup = ImGui::ButtonHoverPopupBegin("Layers", buttrec, popup_size);
	if (popup) {
		if (ImGui::Button("Add New", ImVec2(150, 30))) {
			project->layers.pushBack(new drawlayer());
		}

		ImGui::Separator();

		for (auto lay : project->layers) {
			ImGui::PushID((int) lay.idx());

			bool higlight = lay.data() == project->active_layer;
			if (higlight) {
				ImGui::PushStyleColor(ImGuiCol_Button, GImGui->Style.Colors[ImGuiCol_ButtonActive]);
			}

			if (ImGui::Button(lay->name.cstr(), ImVec2(100, 30))) {
				project->active_layer = lay.data();
			}

			if (higlight) {
				ImGui::PopStyleColor();
			}


			if (ImGui::BeginPopupContextItem(lay->name.cstr(), ImGuiPopupFlags_MouseButtonRight)) {
				static char name[150] = {"asdas"};
				if (ImGui::InputTextEx(" ", "new name", name, 150, {150 , 30}, ImGuiInputTextFlags_EnterReturnsTrue)) {
					lay->name = name;
					lay->name.capture();
				}
				if (ImGui::Button("Delete", ImVec2(150, 30))) {
					project->active_layer = lay.node()->prev ? lay.node()->prev->data : NULL;
					delete lay.node()->data;
					project->layers.delNode(lay.node());
					ImGui::EndPopup();
					ImGui::PopID();
					break;
				}
				if (lay.node()->prev && ImGui::Button("Move Up", ImVec2(150, 30))) {
					swap(lay.node()->prev->data, lay.data());
				}
				if (lay.node()->next && ImGui::Button("Move Down", ImVec2(150, 30))) {
					swap(lay.node()->next->data, lay.data());
				}
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			bool visable = !lay->hiden;
			ImGui::Checkbox("", &visable);
			lay->hiden = !visable;

			ImGui::PopID();
		}
	}
	HoverPopupEnd(popup);

	if (!project->active_layer) {
		ImGui::End();
		return;
	}

	ImGui::SameLine();
	popup = ImGui::ButtonHoverPopupBegin("Hist", buttrec, vec2f(200, 300));
	if (popup) {
		if (ImGui::Button("Undo", ImVec2(100, 30))) {
			project->active_layer->undo();
		}
		if (ImGui::Button("Redo", ImVec2(100, 30))) {
			project->active_layer->redo();
		}
		if (ImGui::Button("Clear History", ImVec2(100, 30))) {
			project->active_layer->clear_history();
		}
	}
	HoverPopupEnd(popup);

	ImGui::SameLine();
	popup = ImGui::ButtonHoverPopupBegin("Tool", buttrec, popup_size);
	if (popup) { draw_brush_properties(rectf(0, 0, 200, 600)); }
	ImGui::HoverPopupEnd(popup);


	ImGui::SameLine();
	popup = ImGui::ButtonHoverPopupBegin("Proj", buttrec, popup_size);
	if (popup) {

		ImGui::SetNextItemWidth(popup_size.x / 2.f);
		int flags = ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs;
		ImGui::ColorPicker4("Background Color", &project->canvas_color.r, flags);

		halni width = ImGui::GetWindowContentRegionWidth();
		if (ImGui::Button("Clear All Strokes", ImVec2(width, 30))) {
			project->active_layer->clear_canvas();
		}

		if (ImGui::Button("Reduce Size", ImVec2(width, 30))) {
			alni length = project->save_size() / (1024);
			project->active_layer->reduce_size(project->pass_factor);
			alni length_after = project->save_size() / (1024);
			ImGui::Notify(sfmt("Size Redused from %i to %i kb", length, length_after).cstr());
		}

		ImGui::SetNextItemWidth(width / 2);
		ImGui::SliderFloat("Pass Facor", &project->pass_factor, 0, 0.01);
	}
	HoverPopupEnd(popup);

	ImGui::SameLine();
	popup = ImGui::ButtonHoverPopupBegin("Cam", buttrec, vec2f(200, 300));
	if (popup) {
		if (ImGui::Button("Reset", ImVec2(100, 30))) {
			project->cam.lookat({0, 0, 0}, {100, 0, 0}, {0, 0, 1});
		}
		halnf fov = project->cam.get_fov();
		ImGui::Text("Field Of View");
		ImGui::SetNextItemWidth(110);
		ImGui::SliderFloat(" ", &fov, 0.1, PI - 0.1);
		project->cam.set_fov(fov);
	}
	HoverPopupEnd(popup);

	ImGui::End();
}

void StrokeApp::gui_draw() {
	/*
	halnf toolbar_size = 290;
	draw_toolbar(rectf(window.size.x / 2.f - toolbar_size / 2.f, window.size.y, toolbar_size, 60));

	if (!project) {
		DrawTextR(rectf(window.size / 2.2, 0), "Select Strokes Object", rgba(0.75, 0.75, 0.75, 1));

	} else if (!gui_is_active || 1) {

		if (project->sampler.eraser) {
			halnf size = project->sampler.eraser_size * window.size.x / 2.f;
			DrawCircle(window.cursor(), size, rgba(0.9, 0.9, 0.9, 0.7), 2.f);
		} else {
			halnf size = project->sampler.screen_thikness / 2.f * window.size.x / 2.f;
			DrawCircle(window.cursor(), size, rgba(0, 0, 0, 0.7), 4.f);
			DrawCircle(window.cursor(), size, project->sampler.stroke_col, 2.f);
		}
	}
}
*/

/*

#pragma once

#include "gl.h"
#include "glutils.h"
#include "window.h"
#include "fbuffer.h"

#include "filesystem.h"

#include "strokes.h"
#include "ImGuiObjectEditor.h"

#include "primitives/primitives.h"
#include "strokesobject.h"

class StrokeApp {

	strokes_project* project = NULL;

	tp::halnf tablet_input_formater[5] = {0.5f, 0.f, 1.f, .5f};

	public:

	StrokeApp(tp::vec2f size = tp::vec2f(1400, 800));
	~StrokeApp();

	private:

	void MainProcTick();
	void MainDrawTick();

	void camera_controller();

	private:

	void gui_draw();
	void draw_explorer();
	void draw_toolbar(tp::rectf rect);
	void draw_brush_properties(tp::rectf rect);
};

using namespace tp;
using namespace obj;

StrokeApp::StrokeApp(vec2f size) {
	//window.col_clear = rgba(0.2, 0.2, 0.23, 1);
	//main_window = false;
	//window.minsize.assign(400, 400);

	//Object* new_scratch = NULL;
	//alni dict_idx = objects_gui.root->items.presents("Scratch");
	//if (dict_idx == -1) {
		//new_scratch = NDO->create("strokes");
		//objects_gui.root->items.put("Scratch", new_scratch);
	//} else {
		//new_scratch = objects_gui.root->items[dict_idx];
	//}
	//objects_gui.cd(new_scratch, "Scratch");
}

void StrokeApp::MainProcTick() {

	if (!objects_gui.active || objects_gui.active->type->name != "strokes") {
		project = NULL;
		window.col_clear = rgba(0.22, 0.22, 0.25, 1);
		return;
	}

	project = &NDO_CAST(StrokesObject, objects_gui.active)->project;

	if (gui_is_active) {
		return;
	}

	if (!project->active_layer) {
		return;
	}

	camera_controller();

	halnf pen_pressure = ImGui::BezierValue(window.pen_pressure(), tablet_input_formater);
	project->sampler.sample(&project->active_layer->strokes, &project->active_layer->strokes_undo, window.cursor(1), pen_pressure, &project->cam);

	if (!project->sampler.active_state() && project->sampler.has_input()) {
		stroke& str = project->sampler.get_stroke();

		str.denoise_positions(project->denoise_passes);
		str.denoise_thickness(project->denoise_passes_thikness);

		if (project->auto_reduction) {
			str.reduce_nof_points(project->pass_factor);
		}

		project->active_layer->add_stroke(project->sampler.get_stroke());
		project->sampler.clear();
	}

	whait_for_event = !project->sampler.active_state();
}

void StrokeApp::MainDrawTick() {
	gui_draw();

	if (project) {
		mat4f cammat = (project->cam.projmat() * project->cam.viewmat()).transpose();
		project->sampler.draw(cammat);

		for (alni idx = project->layers.length() - 1; idx > -1; idx--) {
			if (!project->layers[idx]->hiden) {
				project->layers[idx]->draw(cammat);
			}
		}

		window.col_clear = project->canvas_color;
	}
}


void StrokeApp::camera_controller() {

	vec2f prevcur = window.prevcursor(1);
	vec2f cur = window.cursor(1);

	vec2f delta = prevcur - cur;
	vec3f target = project->cam.get_target();

	if (glfwGetKey(window.geth(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
		project->cam.rotate(delta.x * 5, delta.y * 5);
	}

	if (glfwGetKey(window.geth(), GLFW_KEY_Z) == GLFW_PRESS) {
		project->cam.zoom((prevcur.y + 1.f) / (cur.y + 1.f));
	}

	if (glfwGetKey(window.geth(), GLFW_KEY_SPACE) == GLFW_PRESS) {
		project->cam.move(cur, prevcur);
	}

	if (glfwGetKey(window.geth(), GLFW_KEY_X) == GLFW_PRESS) {
		project->pickTargetLength(cur);
	}

	if (glfwGetKey(window.geth(), GLFW_KEY_B) == GLFW_PRESS) {
		project->sampler.eraser = false;
	}

	if (glfwGetKey(window.geth(), GLFW_KEY_E) == GLFW_PRESS) {
		project->sampler.eraser = true;
	}

	project->cam.set_ratio(window.aspect_ratio());
}

StrokeApp::~StrokeApp() {}
*/
 

/*

struct StrokesRootWidget : nd::Widget {

	static obj::ObjectType TypeData;
	static struct Vtable : Widget::Vtable {} vtable;

	static void constructor(StrokesRootWidget* self) {
	}

	static void destructor(StrokesRootWidget* self) {}
	static void copy(StrokesRootWidget* self, const StrokesRootWidget* in) {}

	static void procInputs(StrokesRootWidget* self, nd::GUI* gui) {

		auto project = &self->strokes->project;
		auto rec = self->getRect();

		//camera_controller();
		auto cursor = gui->gui_window.mNativeWindow.mEvents.mCursor;

		cursor.y = rec.size.y - cursor.y;
		cursor -= tp::vec2f(rec.size) / 2.f;
		cursor /= tp::vec2f(rec.size) / 2.f;

		//bool = gui->gui_window.mNativeWindow.mEvents.mLMouseDown;
		tp::alnf pressure = gui->gui_window.mNativeWindow.mEvents.mLMouseDown;
		//tp::halnf pen_pressure = ImGui::BezierValue(window.pen_pressure(), tablet_input_formater);
		project->sampler.sample(&project->active_layer->strokes, &project->active_layer->strokes_undo, cursor, pressure, &project->cam);

		if (!project->sampler.active_state() && project->sampler.has_input()) {
			stroke& str = project->sampler.get_stroke();

			str.denoise_positions(project->denoise_passes);
			str.denoise_thickness(project->denoise_passes_thikness);

			if (project->auto_reduction) {
				str.reduce_nof_points(project->pass_factor);
			}

			project->active_layer->add_stroke(project->sampler.get_stroke());
			project->sampler.clear();
		}
	}

	static void presentOutput(StrokesRootWidget* self, nd::GUI* gui) {
		if (!self->retrieveProject()) {
			return;
		}

		tp::rect<tp::alni> rec = self->getRect();
		rec.pos = 0;

		glViewport(0, 0, rec.z, rec.w);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto project = &self->strokes->project;

		tp::mat4f cammat = (project->cam.projmat() * project->cam.viewmat()).transpose();
		project->sampler.draw(cammat);

		for (auto lay : project->layers) {
			if (!lay->hiden) {
				lay->draw(cammat);
			}
		}

		//window.col_clear = project->canvas_color;
	}

};

*/


/*

// ----------------------------- strokes ------------------------ //


#pragma once

#include "glcommon.h"
#include "shader.h"

#include "array.h"
#include "list.h"
#include "map.h"
#include "topology.h"
#include "strings.h"

struct StrokeMesh {
	tp::Array<tp::vec3f> vbo;

	tp::ogl::shader* shader;
	tp::rgba color = tp::rgba(1);

	GLuint VertexArrayID;
	GLuint vertexbuffer;
	GLuint MatrixID;
	GLuint ColorID;

	void init();

	StrokeMesh();
	void operator=(const StrokeMesh& in);

	void bind_buffers();
	void draw_mesh(const tp::mat4f& cammat);
	~StrokeMesh();
};

struct stroke_point {
	tp::vec3f pos;
	tp::vec3f normal;
	tp::halnf thikness;
	stroke_point();
};

struct stroke {

	tp::Array<stroke_point> points;

	StrokeMesh mesh;

	void gen_quad(tp::alni pidx, stroke_point* p1, stroke_point* p2, tp::vec3f dir1, tp::vec3f dir2);
	tp::vec3f split_dir(tp::vec3f v1, tp::vec3f v2, const tp::vec3f& norm);

	void gen_mesh();

	void drawcall(const tp::mat4f& cammat);
	void add_point(const stroke_point& p);


};

class drawlayer {
	public:
	tp::string name = "new layer";
	tp::List<stroke> strokes;
	tp::List<stroke> strokes_undo;
	bool hiden = false;

	void undo();
	void redo();

	void add_stroke(const stroke& str);
	void draw(const tp::mat4f& cammat);

	void clear_history() {
		strokes_undo.free();
	}

	void clear_canvas() {
		tp::alni len = strokes.length();
		for (tp::alni idx = 0; idx < len; idx++) {
			undo();
		}
	}

	void reduce_size(tp::halnf pass_factor) {
		for (auto str : strokes) {
			str.data().reduce_nof_points(pass_factor);
			str.data().gen_mesh();
		}
	}

	void clear() {
		strokes.free();
		strokes_undo.free();
	}
};

class inputsmpler {
	public:

	bool is_active = false;

	stroke input;
	tp::halnf pressure;

	tp::rgba stroke_col = tp::rgba(0.77, 0.77, 0.77, 1);

	tp::halnf screen_precision = 0.000f;
	tp::halnf precision = 0.002;
	tp::halnf screen_thikness = 0.01f;
	tp::halnf thickness = 0.04;

	bool eraser = false;
	tp::halnf eraser_size = 0.1f;

	void add_point(const tp::vec3f& pos, const tp::vec3f& norm, float thickness);
	bool passed(const tp::vec3f& point);
	void start(const tp::vec2f& cpos, tp::Camera* cam);
	void sample_util(const tp::vec2f& cpos, tp::Camera* cam);
	void erase_util(tp::List<stroke>* pull, tp::List<stroke>* undo, const tp::vec2f& cpos, tp::Camera* cam);
	void finish(const tp::vec2f& cpos, tp::Camera* cam);

	void sample(tp::List<stroke>* pool, tp::List<stroke>* undo, tp::vec2f curs, float pressure, tp::Camera* cam);
	void draw(const tp::mat4f& cammat);

	bool active_state();
	bool has_input();
	void clear();
	stroke& get_stroke();
};

struct strokes_project {

	tp::Camera cam;
	drawlayer* active_layer = NULL;
	tp::List<drawlayer*> layers;
	inputsmpler sampler;
	tp::rgba canvas_color = tp::rgba(0.22f, 0.22f, 0.25f, 1.f);

	tp::halni denoise_passes = 1;
	tp::halni denoise_passes_thikness = 3;
	bool auto_reduction = true;
	tp::halnf pass_factor = tp::halnf(0.001);

	strokes_project() {
		cam.lookat({0, 0, 0}, {100, 0, 0}, {0, 0, 1});
	}

	drawlayer* get_base_layer() {
		if (!layers.length()) {
			layers.pushBack(new drawlayer());
		}
		return layers[0];
	}

	void append_layers(drawlayer* base) {
		for (auto lay : layers) {
			base->strokes += lay->strokes;
		}
	}

	void pickTargetLength(const tp::vec2f& cur) {
		using namespace tp;
		stroke_point* target_point = NULL;
		alnf min_length = FLT_MAX;
		alnf min_tollerance = 0.05f;

		for (auto lay : layers) {
			for (auto stroke : lay->strokes) {
				for (auto pnt : stroke.data().points) {

					alnf tollerance = (cam.project(pnt.data().pos) - cur).length();
					alnf len = (pnt->pos - cam.get_pos()).dot(cam.get_fw());

					alnf toll2 = abs(min_tollerance - tollerance);
					bool selected = (toll2 < 0.01) ? (min_length > len) : (min_tollerance > tollerance);

					if (selected) {
						target_point = &pnt.data();
						min_length = len;
						min_tollerance = tollerance;
					}
				}
			}
		}

		if (target_point) {
			cam.lookat(cam.get_pos() + (cam.get_fw() * min_length), cam.get_pos(), cam.get_up());
		}
	}

	tp::alni save_size();
	void save(tp::File& file);
	void load(tp::File& file);

	~strokes_project() {
		for (auto lay : layers) {
			delete lay.data();
		}
	}
};



#include "strokes.h"

#include "glutils.h"

#include "StrokesVersions.h"

using namespace tp;

void StrokeMesh::init() {
	static ogl::shader shader("rsc/shaders/stroke", NULL, "rsc/shaders/stroke");
	this->shader = &shader;

	MatrixID = shader.getu("MVP");
	ColorID = shader.getu("StrokeColor");

	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glGenBuffers(1, &vertexbuffer);
}

StrokeMesh::StrokeMesh() {
	init();
}

void StrokeMesh::operator=(const StrokeMesh& in) {

	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);

	init();

	vbo = in.vbo;
	color = in.color;

	bind_buffers();
}

void StrokeMesh::bind_buffers() {

	glBindVertexArray(VertexArrayID);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vbo[0]) * vbo.length(), vbo.buff(), GL_STATIC_DRAW);
}


void StrokeMesh::draw_mesh(const mat4f& cammat) {

	glBindVertexArray(VertexArrayID);

	shader->bind();
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &cammat[0][0]);
	glUniform4fv(ColorID, 1, &color.rgbs.r);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

	// Draw the triangles ! // mode. count. type. element. array buffer offset
	glDrawArrays(GL_TRIANGLES, 0, vbo.length());

	glDisableVertexAttribArray(0);

	shader->unbind();
}

StrokeMesh::~StrokeMesh() {
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
}


stroke_point::stroke_point() {
	pos = vec3f(0, 0, 0);
	normal = vec3f(0, 1, 0);
	thikness = 0.06;
}

void stroke::gen_quad(alni pidx, stroke_point* p1, stroke_point* p2, vec3f dir1, vec3f dir2) {
		//vec3 perp = normalize(cross(p1->normal, p2->pos - p1->pos)) * p1->thikness;
		//float cosa1 = dot(perp, dir1) / (dir1.length() * perp.length());
		//float cosa2 = dot(perp, dir2) / (dir2.length() * perp.length());
		//float thickness1 = -p1->thikness / cosa1;
		//float thickness2 = -p2->thikness / cosa2;

vec3f v1 = p1->pos + dir1 * p1->thikness;
vec3f v2 = p2->pos + dir2 * p2->thikness;
vec3f v3 = p2->pos - dir2 * p2->thikness;
vec3f v4 = p1->pos - dir1 * p1->thikness;

alni vidx = pidx * 6;

mesh.vbo[vidx] = v1;
mesh.vbo[vidx + 1] = v2;
mesh.vbo[vidx + 2] = v3;
mesh.vbo[vidx + 3] = v3;
mesh.vbo[vidx + 4] = v4;
mesh.vbo[vidx + 5] = v1;
}

vec3f stroke::split_dir(vec3f v1, vec3f v2, const vec3f& norm) {

	v1.normalize();
	v2.normalize();

	vec3f plane_normal;

	halnf val = v2.dot(v1);

	if (val > -0.999999) {
		vec3f crossp = v1.cross(v2);
		mat3f rot_matrix = mat3f::rotmat(crossp, acos(v1.dot(v2)) / 2.f);
		vec3f middle = rot_matrix * v1;
		plane_normal = middle.cross(crossp);
	}
	else {
		plane_normal = v2;
	}

	return plane_normal.cross(norm);
}

void stroke::gen_mesh() {

	alni nvert = (points.length() - 1) * 6;

	mesh.vbo.reserve(nvert);

	for (alni pidx = 0; pidx < points.length() - 1; pidx++) {

		stroke_point pt0;
		stroke_point pt1 = points[pidx];
		stroke_point pt2 = points[pidx + 1];
		stroke_point pt3;

		vec3f dir1;
		vec3f dir2;

		if (pidx > 0) {
			pt0 = points[pidx - 1];
		}
		else {
			pt0.pos = pt1.pos + (pt1.pos - pt2.pos);
			pt0.thikness = 0.001;
		}

		if (pidx < points.length() - 2) {
			pt3 = points[pidx + 2];
		}
		else {
			pt3.pos = pt2.pos + (pt2.pos - pt1.pos);
			pt3.thikness = 0.001;
		}

		dir1 = split_dir(pt0.pos - pt1.pos, pt2.pos - pt1.pos, pt1.normal);
		dir2 = split_dir(pt1.pos - pt2.pos, pt3.pos - pt2.pos, pt2.normal);

		if (dir1.dot(dir2) < 0) {
			dir1 *= -1;
		}

		gen_quad(pidx, &pt1, &pt2, dir1.unitv(), dir2.unitv());
	}

	mesh.bind_buffers();
}

void stroke::drawcall(const mat4f& cammat) {
	mesh.draw_mesh(cammat);
}

void stroke::add_point(const stroke_point& p) {
	points.pushBack(p);
}


void drawlayer::undo() {
	if (strokes.last()) {
		strokes_undo.pushBack(strokes.last()->data);
		strokes.delNode(strokes.last());
	}
}

void drawlayer::redo() {
	if (strokes_undo.last()) {
		strokes.pushBack(strokes_undo.last()->data);
		strokes_undo.delNode(strokes_undo.last());
	}
}

void drawlayer::add_stroke(const stroke& str) {
	strokes.pushBack(str);
	strokes.last()->data.gen_mesh();
}

void drawlayer::draw(const mat4f& cammat) {
	for (ListNode<stroke>* str = strokes.last(); str; str = str->prev) {
		str->data.drawcall(cammat);
	}
}

void inputsmpler::add_point(const vec3f& pos, const vec3f& norm, float thickness) {
	stroke_point p;
	p.pos = pos;
	p.normal = norm;
	p.thikness = thickness * pressure;
	input.add_point(p);
}

bool inputsmpler::passed(const vec3f& point) {
	if (input.points.length()) {
		return (point - input.points[input.points.length() - 1].pos).length() > precision;
	}
	return true;
}

void inputsmpler::start(const vec2f& cpos, Camera* cam) {
	input.points.free();
	vec3f point = cam->project(cpos);
	add_point(point, cam->get_fw(), 0.001f);
}

void inputsmpler::sample_util(const vec2f& cpos, Camera* cam) {
	vec3f point = cam->project(cpos);
	if (passed(point)) {
		add_point(point, cam->get_fw(), thickness);
	}
}

void inputsmpler::erase_util(List<stroke>* pull, List<stroke>* undo, const vec2f& cpos, Camera* cam) {

	ListNode<stroke>* str = pull->first();
	while (str) {
		bool remove = false;

		for (auto pnt : str->data.points) {
			if ((cam->project(pnt.data().pos) - cpos).length() < eraser_size) {
				remove = true;
				break;
			}
		}

		if (remove) {
			ListNode<stroke>* tmp = str->next;
			undo->pushBack(str->data);
			pull->delNode(str);
			str = tmp;
		}
		else {
			str = str->next;
		}
	}
}

void inputsmpler::finish(const vec2f& cpos, Camera* cam) {
	if (input.points.length() <= 1) {
		input.points.free();
	}
	else {
		input.points[input.points.length() - 1].thikness = 0.001f;
		cam->offset_target(0.0001);
	}
}

void inputsmpler::sample(List<stroke>* pull, List<stroke>* undo, vec2f curs, float pressure, Camera* cam) {

	this->input.mesh.color = stroke_col;
	this->pressure = pressure;

	if (eraser) {
		if (pressure > 0) {
			input.points.free();
			erase_util(pull, undo, curs, cam);
		}
		return;
	}
	else {
		vec3f zero_projection = cam->project(vec2f(0.f));
		thickness = (cam->project(vec2f(0.f, screen_thikness)) - zero_projection).length();
		precision = (cam->project(vec2f(0.f, screen_precision)) - zero_projection).length();
	}

	switch (is_active) {
	case false:
	{
		if (pressure) {
			start(curs, cam);
			is_active = true;
		}
		return;
	}
	case true:
	{
		sample_util(curs, cam);
		if (!pressure) {
			finish(curs, cam);
			is_active = false;
			return;
		}
		return;
	}
	}
}

void inputsmpler::draw(const mat4f& cammat) {

	static int prev_len = input.points.length();
	if (input.points.length() > 1) {
		if (prev_len < input.points.length()) {
			input.gen_mesh();
		}
		input.drawcall(cammat);
	}
	prev_len = input.points.length();
}

bool inputsmpler::active_state() {
	return is_active;
}
bool inputsmpler::has_input() {
	return input.points.length() > 1;
}
void inputsmpler::clear() {
	input.points.free();
	is_active = false;
}
stroke& inputsmpler::get_stroke() {
	return input;
}


alni strokes_project::save_size() {
	alni version_size = StrokesVersion1::save_size(this);
	return version_size + sizeof(ProjectInfo);
}

void strokes_project::save(File& file) {
	ProjectInfo head;
	head.init(1);
	file.write<ProjectInfo>(&head);
	StrokesVersion1::save(file, this);
}

void strokes_project::load(File& file) {
	ProjectInfo head;
	file.read<ProjectInfo>(&head);

	if (string(head.name) != "strokes") {
		return;
	}

	alni version = alni(string(head.version));
	if (version > 1 || version < 0) {
		return;
	}

	version == 1 ? StrokesVersion1::load(file, this) : StrokesVersion0::load(file, this);

	active_layer = get_base_layer();
}

*/
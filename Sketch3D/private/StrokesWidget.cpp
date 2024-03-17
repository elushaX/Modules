
#include "StrokesWidget.h"
#include "operator.h"

using namespace nd;

void StrokesWidget::constructor(StrokesWidget* self) {
	new (&self->mRenderer) strokes::Renderer({ 1980, 1020 });
	self->createMember("link", "TargetProject");
	self->createMember("StrokesProject", "default");
	self->createMember("rgba", "draw_color");
	self->addMember(obj::ColorObject::create({ 0.13f, 0.13f, 0.13f, 1.f }), "bg_color");
}

void StrokesWidget::destructor(StrokesWidget* self) {
	self->mRenderer.~Renderer();
	self->mImage.free(self->mDrawer);
	self->mImage.~ImageHandle();
}

void StrokesWidget::copy(Widget* self, const Widget* in) {}

strokes::Project* StrokesWidget::getTargetProject() {
	auto link = getMember<obj::LinkObject>("TargetProject");
	if (!link) {
		return NULL;
	}
	auto obj = link->getLink();
	if (!obj) {
		link->setLink(getMember<obj::StrokesObject>("default"));
		return getTargetProject();
	}
	auto strokes = NDO_CAST(obj::StrokesObject, obj);
	if (!strokes) {
		return NULL;
	}

	return &strokes->mProject;
}

void StrokesWidget::procInputs(StrokesWidget* self, nd::GUIInputs* inputs) {
	auto proj = self->getTargetProject();
	if (!proj) {
		return;
	}

	auto rec = self->getRect();
	proj->mCamera.set_ratio(rec.w / rec.z);

	auto col_obj_bg = self->getMember<obj::ColorObject>("bg_color");
	if (col_obj_bg) {
		proj->mBackgroundColor = col_obj_bg->mCol;
	}

	if (!rec.inside(inputs->mCrsPrev)) {
		return;
	}

	auto pressure = inputs->mPressure;
	if (!inputs->Anticipating()) {
		pressure = 0.f;
	}

	auto idx = proj->mBrushes.presents(proj->mActiveBrush);
	if (idx) {
		auto brush = proj->mBrushes.getSlotVal(idx);
		auto crs = (inputs->mCrs - rec.pos);
		crs.x /= rec.z;
		crs.y /= rec.w;
		crs = (crs - 0.5) * 2;

		if (brush->mType == "pencil") {
			auto col_obj = self->getMember<obj::ColorObject>("draw_color");
			if (col_obj) {
				((strokes::PencilBrush*)brush)->mCol = col_obj->mCol;
			}
		}

		brush->sample(proj, crs, pressure);
	}
}

void StrokesWidget::presentOutput(StrokesWidget* self, nd::GUIdrawer* drawer) {
	auto proj = self->getTargetProject();
	if (!proj) {
		return;
	}

	if (!self->mImage.mId) {
		self->mDrawer = drawer;
		self->mImage.createFromBuff(self->mRenderer.getBuff(), drawer);
	}

	auto rec = self->getRect();
	self->mRenderer.setViewport({ 0, 0, rec.z, rec.w });
	self->mRenderer.setClearCol(proj->mBackgroundColor);
	self->mRenderer.renderBegin();
	
	for (auto lay : proj->mLayers) {
		if (lay.data()->enabled) {
			for (auto str : lay.data()->strokes) {
				self->mRenderer.drawStroke(str.data(), &proj->mCamera);
			}
		}
	}
	
	auto idx = proj->mBrushes.presents(proj->mActiveBrush);
	if (idx) {
		auto brush = proj->mBrushes.getSlotVal(idx);
		brush->draw(&self->mRenderer, &proj->mCamera);
	}

	self->mRenderer.renderEnd();
	
	drawer->drawImage(rec, &self->mImage, 0, 1, 12);
}

struct obj::ObjectType StrokesWidget::TypeData = {
	.base = &Widget::TypeData,
	.constructor = (obj::object_constructor)StrokesWidget::constructor,
	.destructor = (obj::object_destructor)StrokesWidget::destructor,
	.copy = (obj::object_copy)StrokesWidget::copy,
	.size = sizeof(StrokesWidget),
	.name = "StrokesWidget",
	.vtable = &StrokesWidget::vtable
};

struct StrokesWidget::Vtable StrokesWidget::vtable = {
	(void (*)(Widget*, nd::GUIInputs*)) & StrokesWidget::procInputs,
	(void (*)(Widget*, nd::GUIdrawer*)) & StrokesWidget::presentOutput,
};


class StrokesWidgetOps : public nd::TypeOperators {

	struct CameraOrbit : public nd::Operator {
		tp::vec2f prev_crs;
		struct Invoke : public nd::OpCallBack {
			Invoke() {
				auto transf = obj::EnumObject::create({ "move", "rotate" , "scale" });
				arg_interface.put("Gui", obj::NDO->create("link"));
				arg_interface.put("transf", transf);

				func = exec;
				description = "invoke";
			}
			static void exec(Operator* op, obj::DictObject* args) {
				auto active = OpCallBack::getActiveWidget<StrokesWidget>(args);
				if (!active) { 
					return; 
				}
				
				auto proj = active->getTargetProject();
				if (!proj) {
					return;
				}

				auto rec = active->getRect();

				auto gui = OpCallBack::getGUI(args);
				auto prev_crs = ((CameraOrbit*)op)->prev_crs;
				prev_crs = tp::vec2f(prev_crs.x / rec.size.x, prev_crs.y / rec.size.y);
				prev_crs = (prev_crs - 0.5f) * 2.f;
				auto crs = tp::vec2f(gui->mInputs.mCrs.x / rec.size.x, gui->mInputs.mCrs.y / rec.size.y);
				crs = (crs - 0.5f) * 2.f;
				auto delta = (crs - prev_crs);

				if (tp::abs(delta.x) < 0.1 && tp::abs(delta.y) < 0.1) {

					auto type = OpCallBack::getArg<obj::EnumObject>(args, "transf");
					switch (type->active) {
						case 0: {
							proj->mCamera.move(crs, prev_crs);
							break;
						}
						case 1: {
							proj->mCamera.rotate(-delta.x * 5, -delta.y * 5);
							break;
						}
						case 2: {
							proj->mCamera.zoom((crs.y + 1.f) / (prev_crs.y + 1.f));
							break;
						}
					}

				}
				((CameraOrbit*)op)->prev_crs = gui->mInputs.mCrs;
			}
		} invoke_callback;

		CameraOrbit() {
			description = "Orbits camera";
			callbacks.put("invoke", &invoke_callback);
		}
	} camera_orbit_op;

public:
	StrokesWidgetOps() {
		operators.put("camera_orbit", &camera_orbit_op);
	}
};

void StrokesWidget::OpsInit() {
	assert(!StrokesWidget::TypeData.nodes_custom_data);
	StrokesWidget::TypeData.nodes_custom_data = new StrokesWidgetOps();
}

void StrokesWidget::OpsUnInit() {
	assert(StrokesWidget::TypeData.nodes_custom_data);
	auto del = (StrokesWidgetOps*)StrokesWidget::TypeData.nodes_custom_data;
	StrokesWidget::TypeData.nodes_custom_data = NULL;
	delete del;
}

void StrokesWidget::addShortcuts(nd::NodesCore* nodes) {
	auto uis_list = nodes->getMemberAssert<obj::ListObject>("uis");
	NDO_CASTV(nd::GUI, uis_list->getItems()[0], gui);
	NDO_CASTV(nd::TUI, uis_list->getItems()[1], tui);

	auto shortcuts = tui->getMemberAssert<obj::ListObject>("shortcuts");
	auto inputs = tui->getMemberAssert<obj::DictObject>("inputs");
	auto input_states = tui->getMemberAssert<obj::DictObject>("input_states");
	auto input_state_hold = input_states->get("hold");

	{ // orbit
		auto shortcut = nd::Shortcut::createShortcut("StrokesWidget", "camera_orbit");
		auto args = NDO_CAST(obj::DictObject, shortcut->callbacks_arguments->get("invoke"));
		NDO_CAST(obj::LinkObject, args->get("Gui"))->setLink(gui);
		NDO_CAST(obj::EnumObject, args->get("transf"))->active = 1;

		shortcuts->pushBack(shortcut);

		auto trigger = shortcut->addTrigger("invoke");
		auto back = tui->getInput(tp::Keycode::LEFT_ALT)->getMember<obj::EnumObject>("state");
		trigger->addComparator("comp", back, input_state_hold);
	}

	{ // zoom
		auto shortcut = nd::Shortcut::createShortcut("StrokesWidget", "camera_orbit");
		auto args = NDO_CAST(obj::DictObject, shortcut->callbacks_arguments->get("invoke"));
		NDO_CAST(obj::LinkObject, args->get("Gui"))->setLink(gui);
		NDO_CAST(obj::EnumObject, args->get("transf"))->active = 2;
		shortcuts->pushBack(shortcut);

		auto trigger = shortcut->addTrigger("invoke");
		auto back = tui->getInput(tp::Keycode::LEFT_CONTROL)->getMember<obj::EnumObject>("state");
		trigger->addComparator("comp", back, input_state_hold);
	}

	{ // pan
		auto shortcut = nd::Shortcut::createShortcut("StrokesWidget", "camera_orbit");
		auto args = NDO_CAST(obj::DictObject, shortcut->callbacks_arguments->get("invoke"));
		NDO_CAST(obj::LinkObject, args->get("Gui"))->setLink(gui);
		shortcuts->pushBack(shortcut);

		auto trigger = shortcut->addTrigger("invoke");
		auto back = tui->getInput(tp::Keycode::SPACE)->getMember<obj::EnumObject>("state");
		trigger->addComparator("comp", back, input_state_hold);
	}
}
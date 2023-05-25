
#include "SimpleGui.h"
#include "debugui.h"
#include "memleaks.h"
#include "timer.h"

struct MemLeaksGUI {

	tp::glw::Window window;
	tp::glw::DebugUI ui;
	tp::glw::Canvas canvas;
	tp::glw::WindowSimpleInputs inputs;
	tp::glw::WindowsHeaderWidget header;
	tp::MemLeaksData mLeaks;
	tp::MemLeaksTreeView mView;
	tp::halnf mHeaderHeight = 5.5f;
	tp::Timer timer;

	MemLeaksGUI(const char* path) : window(), ui(window, debuguiCallBack, this), mLeaks(path), timer(10) {
		canvas.setCol1({ 0.5f, 0.5f, 0.5f, 0.5f });
		window.mAppearence.mHiden = false;
		mView.setTarget(&mLeaks);
	}

	void proc() {
		window.pollEvents();
		inputs.update(window, 1.f);

		header.header_rec = { (inputs.mWindowSizeMM.x - 60.f) / 2.f, 3.f, 60.f, mHeaderHeight };
		mView.rect = { 0.f, 0.f, inputs.mWindowSizeMM.x, inputs.mWindowSizeMM.y };

		mView.proc();

		if (inputs.MoveUp()) {
			mView.zoom_factor -= 0.2f;
		}
		else if (inputs.MoveDown()) {
			mView.zoom_factor += 0.2f;
		}
		mView.zoom_factor = tp::clamp(mView.zoom_factor, 0.f, 1.f);

		mView.vieport_crs = inputs.mCrs;
		mView.vieport_crs_delta = inputs.mCrsmDelta * -1.f;
		mView.mouse_down = inputs.Activated();
		mView.mouse_hold = inputs.Anticipating();
		mView.mouse_up = inputs.Selected();
		
		header.proc(window, inputs);
	}

	void draw() {
		window.beginDraw(); {
			canvas.beginDraw({ { 0, 0 },  inputs.mWindowSizeMM }, window.mDevice.mDPMM, 1.f); {
				mView.draw(&canvas);
				header.draw(canvas);
			}	canvas.endDraw();
			ui.drawDebugUI(window.mDevice.mDPMM);
		}	window.endDraw();
	}

	void run() {
		while (!window.mEvents.mClose) {
			proc();
			if (window.mEvents.mRedraw) {
				draw();
			}

			timer.wait();
			timer.reset();
		}
	}

	void debugui() {}
	static void debuguiCallBack(void* self) { ((MemLeaksGUI*)self)->debugui(); }
	~MemLeaksGUI() {}
};

int main(char argc, char* argv[]) {
	tp::set_working_dir();

	tp::ModuleManifest* ModuleDependencies[] = { &tp::gModuleGlw, NULL };
	tp::ModuleManifest TestModule("Test", NULL, NULL, ModuleDependencies);
	if (!TestModule.initialize()) {
		return 1;
	}
	
	{
		MemLeaksGUI gui(argc > 1 ? argv[1] : "rsc/debug.memleaks");
		gui.run();
	}

	TestModule.deinitialize();
}
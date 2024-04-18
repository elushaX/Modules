
#include "EditorWidget.hpp"

#include "GraphicApplication.hpp"

using namespace tp;

bool loadMeshes(tp::Scene& scene, const std::string& objetsPath);

class EditorGUI : public Application {
public:
	EditorGUI() {
		Vec2F renderResolution = { 1000, 1000 };
		auto canvas = this->mGraphics->getCanvas();
		// mGui = new EditorWidget<EventHandler, Canvas>(canvas, &geometry, renderResolution);

		mGui = new ShortcutsTest<EventHandler, Canvas>();
			
		loadMeshes(geometry, "rsc/scene.obj");

		geometry.mCamera.lookAtPoint({ 0, 0, 0 }, { 3, 3, 2 }, { 0, 0, 1 });
	}

	~EditorGUI() override { delete mGui; }

	void processFrame(EventHandler* eventHandler) override {
		auto rec = RectF({ 0, 0 }, mWindow->getSize());
		mGui->proc(*eventHandler, rec, rec);
	}

	void drawFrame(Canvas* canvas) override { mGui->draw(*canvas); }

private:
	Scene geometry;
	ShortcutsTest<EventHandler, Canvas>* mGui;
};

int main() {
	EditorGUI gui;
	gui.run();
}

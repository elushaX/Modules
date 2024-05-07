
#include "EditorWidget.hpp"

#include "GraphicApplication.hpp"

using namespace tp;

bool loadMeshes(tp::Scene& scene, const std::string& objetsPath);

class EditorGUI : public Application {
public:
	EditorGUI() {
		Vec2F renderResolution = { 1000, 1000 };
		auto canvas = this->mGraphics->getCanvas();
		mGui = new EditorWidget<EventHandler, Canvas>(canvas, &geometry, renderResolution);

		loadMeshes(geometry, "rsc/scene.obj");

		geometry.mCamera.lookAtPoint({ 0, 0, 0 }, { 3, 3, 2 }, { 0, 0, 1 });
	}

	~EditorGUI() override { delete mGui; }

	void processFrame(EventHandler* eventHandler) override {
		auto rec = RectF({ 0, 0 }, mWindow->getSize());

		mGui->setArea(rec);
		mGui->setVisible(true);
		mGui->updateConfigWrapper(mWidgetManager);
		mGui->procWrapper(*eventHandler, rec);
	}

	void drawFrame(Canvas* canvas) override { mGui->drawWrapper(*canvas); }

private:
	Scene geometry;
	WidgetManager mWidgetManager;
	EditorWidget<EventHandler, Canvas>* mGui;
};

int main() {
	EditorGUI gui;
	gui.run();
}

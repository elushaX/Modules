
#include "EditorWidget.hpp"

#include "GraphicApplication.hpp"

using namespace tp;

class EditorGUI : public Application {
public:
	EditorGUI() {
		Vec2F renderResolution = { 1000, 1000 };
		auto canvas = this->mGraphics->getCanvas();
		mGui = new EditorWidget<EventHandler, Canvas>(canvas, &geometry, renderResolution);
		//
	}

	~EditorGUI() override { delete mGui; }

	void processFrame(EventHandler* eventHandler) override {
		auto rec = RectF({ 0, 0 }, mWindow->getSize());
		mGui->proc(*eventHandler, rec, rec);
	}

	void drawFrame(Canvas* canvas) override { mGui->draw(*canvas); }

private:
	Geometry geometry;
	EditorWidget<EventHandler, Canvas>* mGui;
};

int main() {
	GlobalGUIConfig mConfig;
	gGlobalGUIConfig = &mConfig;

	{
		EditorGUI gui;
		gui.run();
	}
}


#include "EditorWidget.hpp"

#include "GraphicApplication.hpp"

using namespace tp;


class EditorGUI : public Application {
public:
	EditorGUI() {
		auto canvas = this->mGraphics->getCanvas();
		mGui = new EditorWidget<EventHandler, Canvas>(canvas, &mEditor);

		mEditor.loadDefaults();

		// mScene.mCamera.lookAtPoint({ 0, 0, 0 }, { 3, 3, 2 }, { 0, 0, 1 });
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
	Editor mEditor;
	WidgetManager mWidgetManager;
	EditorWidget<EventHandler, Canvas>* mGui;
};

int main() {
	EditorGUI gui;
	gui.run();
}

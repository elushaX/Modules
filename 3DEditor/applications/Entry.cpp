
#include "EditorWidget.hpp"

#include "WidgetApplication.hpp"
#include "RootWidget.hpp"

using namespace tp;

class EditorGUI : public WidgetApplication {
public:
	EditorGUI() {
		auto canvas = this->mGraphics->getCanvas();
		mGui = new EditorWidget(canvas, &mEditor);

		mEditor.loadDefaults();

		setRoot(mGui);
		// mScene.mCamera.lookAtPoint({ 0, 0, 0 }, { 3, 3, 2 }, { 0, 0, 1 });
	}

	~EditorGUI() override { delete mGui; }

private:
	Editor mEditor;
	EditorWidget* mGui;
};

int main() {
	EditorGUI gui;
	gui.run();
}

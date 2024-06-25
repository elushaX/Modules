
#include "SimpleGUI.hpp"

#include "GraphicApplication.hpp"


using namespace tp;

class SimpleGUI : public Application {
public:
	SimpleGUI() {

		// debug

		mGui.addSideWidget(&mButtons[0], DockSpaceWidget::BOTTOM);
		mGui.addSideWidget(&mButtons[1], DockSpaceWidget::RIGHT);

		mGui.removeSideWidget(DockSpaceWidget::BOTTOM);

		mGui.addSideWidget(&mButtons[0], DockSpaceWidget::TOP);
		mGui.addSideWidget(&mButtons[2], DockSpaceWidget::LEFT);

		mGui.removeSideWidget(DockSpaceWidget::TOP);

		mGui.addSideWidget(&mButtons[0], DockSpaceWidget::BOTTOM);

		mGui.addSideWidget(&mButtons[3], DockSpaceWidget::TOP);
		mGui.setCenterWidget(&mButtons[4]);

		mButtons[4].mCallback = [&]() { mGui.toggleHiddenState(DockSpaceWidget::BOTTOM); };
		mButtons[0].mCallback = [&]() { mGui.toggleHiddenState(DockSpaceWidget::TOP); };
		mButtons[2].mCallback = [&]() { mGui.toggleHiddenState(DockSpaceWidget::RIGHT); };
		mButtons[3].mCallback = [&]() { mGui.toggleHiddenState(DockSpaceWidget::LEFT); };
	}

	void processFrame(EventHandler* eventHandler) override {
		const auto rec = RectF({ 0, 0 }, mWindow->getSize());

		mGui.setArea(rec);
		mGui.updateConfigWrapper(mWidgetManager);
		mGui.procWrapper(*eventHandler, rec);
	}

	void drawFrame(Canvas* canvas) override {
		canvas->rect(mGui.mArea, { 0.1f, 0.1f, 0.1f, 1.f });
		mGui.drawWrapper(*canvas);
	}

private:
	WidgetManager mWidgetManager;

	DockSpaceWidget mGui;

	ButtonWidget mButtons[5];
};

int main() {
	{
		SimpleGUI gui;
		gui.run();
	}
}

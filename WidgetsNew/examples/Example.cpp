
#include "GraphicApplication.hpp"

#include "RootWidget.hpp"
#include "FloatingWidget.hpp"
#include "DockWidget.hpp"

#include "ScrollableLayout.hpp"

using namespace tp;


class WidgetApplication : public Application {
public:
	WidgetApplication() {
		exampleAll();
	}

	void exampleAll() {
		static DockWidget dock;
		static LabelWidget centralWidget;
		static FloatingMenu menus[4];
		static FloatingMenu nestedMenus[4];
		static FloatingMenu buttons[10];

		dock.setCenterWidget(&centralWidget);

		for (auto& menu : menus) {
			dock.addChild(&menu);
		}

		for (auto& menu : nestedMenus) {
			menus[0].addToMenu(&menu);
		}

		mRootWidget.setRootWidget(&dock);
	}

	void exampleScrolling() {
		static Widget widget;
		static Widget content;
		static ButtonWidget buttons[10];
		static ScrollableBarWidget scrollBar;

		mRootWidget.setRootWidget(&widget);

		widget.addChild(&scrollBar);
		widget.addChild(&content);

		for (auto& button : buttons) {
			content.addChild(&button);
		}

		widget.setLayout(new ScrollableLayout(&widget));

		RootWidget::setWidgetArea(content, { 333, 333, 522, 522 });
	}

	void examplePopup() {
		static Widget root;
		static ButtonWidget closeButton;
		static ButtonWidget openButton;

		root.addChild(&openButton);
		((BasicLayout*)root.getLayout())->setLayoutPolicy(LayoutPolicy::Passive);

		openButton.setAction([&](){
			closeButton.setArea({ 50, 50, 100, 100 });
			openButton.openPopup(&closeButton);
		});

		closeButton.setAction([&](){
			closeButton.closePopup(&closeButton);
		});

		openButton.setText("open");
		closeButton.setText("close");

		RootWidget::setWidgetArea(openButton, { 333, 333, 222, 222 });

		mRootWidget.setRootWidget(&root);
	}

	void exampleNestedMenus() {
		static DockWidget dock;
		static FloatingMenu menu1;
		static FloatingMenu menu2;
		static ButtonWidget buttons[15];

		mRootWidget.setRootWidget(&dock);

		dock.addChild(&menu1);
		dock.addChild(&menu2);

		dock.setCenterWidget(&buttons[5]);
		dock.dockWidget(&buttons[6], DockLayout::RIGHT);

		menu1.addToMenu(&buttons[0]);

		menu2.addToMenu(&buttons[2]);
		menu2.addToMenu(&buttons[3]);
		menu2.addToMenu(&buttons[4]);

		menu2.addToMenu(&menu1);

		// popup
		{
			buttons[0].setAction([&](){
				buttons[10].setArea({ 30, 30, 100, 100 });
				buttons[0].openPopup(&buttons[10]);
			});

			buttons[0].setText("open popup");

			buttons[10].setAction([&](){
				buttons[10].closePopup(&buttons[10]);
			});

			buttons[10].setText("close popup");
		}

		RootWidget::setWidgetArea(menu1, { 300, 100, 150, 500 });
		RootWidget::setWidgetArea(menu2, { 100, 100, 150, 300 });
	}

	void exampleLayouts() {
		static Widget widgets[15];
		static ButtonWidget buttons[15];

		mRootWidget.setRootWidget(&widgets[0]);

		widgets[0].addChild(&widgets[1]);

		widgets[1].addChild(&buttons[1]);
		widgets[1].addChild(&buttons[2]);
		widgets[1].addChild(&widgets[2]);

		RootWidget::setWidgetArea(buttons[1], { 300, 100, 350, 800 });
		RootWidget::setWidgetArea(buttons[2], { 100, 100, 150, 300 });

		widgets[2].addChild(&buttons[3]);
		widgets[2].addChild(&buttons[4]);
		widgets[2].addChild(&buttons[5]);
		widgets[2].addChild(&buttons[6]);

		buttons[1].setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);
		buttons[2].setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);
		buttons[3].setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);
		buttons[4].setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);
		buttons[5].setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);
		buttons[6].setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);

		widgets[2].setSizePolicy(SizePolicy::Expanding, SizePolicy::Expanding);
		((BasicLayout*)widgets[1].getLayout())->setLayoutPolicy(LayoutPolicy::Horizontal);
	}

	void exampleDock() {
		static DockWidget dock;
		static FloatingMenu menu1;
		static FloatingMenu menu2;
		static ButtonWidget buttons[15];
		
		mRootWidget.setRootWidget(&dock);

		dock.addChild(&menu2);
		dock.addChild(&menu1);

		dock.setCenterWidget(&buttons[5]);
		dock.dockWidget(&buttons[6], DockLayout::RIGHT);

		menu1.addToMenu(&buttons[0]);
		menu2.addToMenu(&buttons[2]);
		//menu2.addToMenu(&menu1);
		menu2.addToMenu(&buttons[3]);
		menu2.addToMenu(&buttons[4]);

		buttons[0].setAction([&]() { buttons[2].setColor(RGBA::random()); });
		buttons[2].setAction([&]() { buttons[0].setColor(RGBA::random()); });

		buttons[5].setAction([&]() { dock.dockWidget(&menu1, DockLayout::LEFT); });
		buttons[6].setAction([&]() { dock.undockWidget(DockLayout::LEFT); });

		buttons[3].setAction([&]() { dock.toggleWidgetVisibility(DockLayout::LEFT); });
		// buttons[4].setAction([this]() { mDockLayout.undockWidget(DockLayout::LEFT); });

		buttons[3].setText("toggle");
		buttons[5].setText("dock");
		buttons[6].setText("undock");

		RootWidget::setWidgetArea(menu2, { 300, 100, 150, 500 });
		RootWidget::setWidgetArea(menu1, { 100, 100, 150, 300 });
	}

	void processFrame(EventHandler* eventHandler, halnf deltaTime) override {
		const auto rec = RectF({ 0, 0 }, mWindow->getSize());
		mRootWidget.processFrame(eventHandler, rec);
	}

	void drawFrame(Canvas* canvas) override {
		mRootWidget.drawFrame(*canvas);

		// drawDebug();
	}

	bool forceNewFrame() override {
		return mRootWidget.needsUpdate();
	}

private:
	RootWidget mRootWidget;
};

int main() {
	{
		WidgetApplication gui;
		gui.run();
	}
}

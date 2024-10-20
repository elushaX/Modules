
#include "WidgetApplication.hpp"

#include "RootWidget.hpp"
#include "FloatingWidget.hpp"
#include "DockWidget.hpp"
#include "ColorPickerWidget.hpp"

#include "ScrollableLayout.hpp"

using namespace tp;


class Example : public WidgetApplication {
public:
	Example() {
		exampleColorPicker();
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

		setRoot(&dock);
	}

	void exampleColorPicker() {
		static DockWidget dock;
		static RGBPickerWidget colorPicker;

		dock.dockWidget(&colorPicker, DockLayout::RIGHT);

		setRoot(&dock);
	}

	void exampleBasic() {
		static Widget widget;
		static LabelWidget label;
		static SliderWidget slider;
		static ButtonWidget button;

		widget.addChild(&label);
		widget.addChild(&button);
		widget.addChild(&slider);

		setRoot(&widget);
	}

	void exampleScrolling() {
		static ScrollableWidget widget;
		static ButtonWidget buttons[10];

		widget.setDirection(false);

		setRoot(&widget);

		for (auto& button : buttons) {
			widget.getContainer()->addChild(&button);
		}

		RootWidget::setWidgetArea(*widget.getContainer(), { 333, 333, 522, 522 });
	}

	void examplePopup() {
		static Widget root;
		static Widget popup;

		static ButtonWidget closeButton;
		static ButtonWidget openButton;
		static ButtonWidget buttons[3];

		for (auto& button : buttons) {
			popup.addChild(&button);
		}

		popup.addChild(&closeButton);

		root.addChild(&openButton);
		((BasicLayout*)root.getLayout())->setLayoutPolicy(LayoutPolicy::Passive);

		openButton.setAction([&](){
			popup.setArea({ 50, 50, 300, 300 });
			openButton.openPopup(&popup);
		});

		closeButton.setAction([&](){
			closeButton.closePopup(&popup);
		});

		openButton.setText("open");
		closeButton.setText("close");

		RootWidget::setWidgetArea(openButton, { 333, 333, 222, 222 });

		setRoot(&root);
	}

	void exampleNestedMenus() {
		static DockWidget dock;
		static FloatingMenu menu1;
		static FloatingMenu menu2;
		static ButtonWidget buttons[15];

		setRoot(&dock);

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

		setRoot(&widgets[0]);

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
		
		setRoot(&dock);

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
};

int main() {
	{
		Example gui;
		gui.run();
	}
}

#pragma once

#include "Containers.hpp"
#include "Menus.hpp"

namespace tp::ui {

	class ApplicationFrame : public Panel {
	public:
		ApplicationFrame();

		void setContent(Widget* content);
		[[nodiscard]] Widget* content() const { return mContent; }

		void setMenuBarVisible(bool visible);
		[[nodiscard]] bool isMenuBarVisible() const { return menuBar.isVisible(); }

		MenuBar menuBar;

	protected:
		[[nodiscard]] Widget* defaultKeyTarget() override { return mContent ? mContent : this; }

	private:
		Widget* mContent = nullptr;
	};
}

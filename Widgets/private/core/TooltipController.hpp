#pragma once

#include "Environment.hpp"

#include <memory>

namespace tp::ui {

	class Ui;
	class Widget;
	class TooltipBubble;

	class TooltipController {
	public:
		explicit TooltipController(Ui& ui);
		~TooltipController();

		void onHoverChanged(Widget* hovered);
		void onPointerActivity();
		void onPress();
		bool showFor(Widget& widget);
		void advance(halnf deltaMs);
		void forget(const Widget& widget);

		[[nodiscard]] bool isWaiting() const { return mCandidate != nullptr && !isShown(); }
		[[nodiscard]] bool isShown() const;
		[[nodiscard]] Widget* bubble() const;

	private:
		void show();
		void hide();

		Ui& mUi;
		std::unique_ptr<TooltipBubble> mBubble;
		Widget* mCandidate = nullptr;
		halnf mDwellMs = 0;
		bool mPinned = false;
	};
}

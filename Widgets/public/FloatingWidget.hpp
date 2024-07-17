#pragma once

#include "CollapsableMenu.hpp"

namespace tp {
	class FloatingWidget : public CollapsableMenu {
	public:
		FloatingWidget();

		void eventProcess(const Events& events) override;
		void eventDraw(Canvas& canvas) override;
		void eventUpdateConfiguration(WidgetManager& wm) override;

		[[nodiscard]] bool isFloating() const;
		void stopFloating();

	private:
		void checkFloating(const Events& events);
		void checkResizing(const Events& events);
		RectF getResizeHandle();

	private:
		Vec2F mMinSize = { 70, 70 };

		halnf mResizeHandleSize = 10;
		RGBA mResizeHandleColor = {};

		bool mFloating = false;
		bool mResizing = false;

		Vec2F mActionStartRelativePos = {};

	public:
		bool mResizable = true;

		bool mDropped = false;
	};
}
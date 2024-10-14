#pragma once

#include "BasicLayout.hpp"

namespace tp {
	class FloatingLayout : public BasicLayout {
	public:
		explicit FloatingLayout(Widget* widget) :
			BasicLayout(widget) {}

	public:
		void startAction(const Vec2F& pointer);
		void updateAction(const Vec2F& pointer);
		void endAction();

		bool isFloating() const { return mIsFloating; }

		RectF resizeHandleRect();

	public:
		void pickRect() override;

	private:
		bool mIsFloating = false;
		bool mIsResizing = false;

		Vec2F mPointerStart;
		Vec2F mPointerCurrent;

		// TODO : remove?
		halnf mHandleSize = 10;
		halnf mHandlePadding = 2;
	};
}
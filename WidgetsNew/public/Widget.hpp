#pragma once

#include "SpringAnimations.hpp"

#include "EventHandler.hpp"
#include "Graphics.hpp"

#include <vector>

namespace tp {
	class WidgetManagerInterface;

	class Widget {
		friend class RootWidget;

	public:
		Widget();
		virtual ~Widget() = default;

		void setDebug(const char* name, RGBA col) {
			mName = name;
			mDebugColor = col;
		}

		void addChild(Widget* child) {
			mChildren.push_back(child);
			child->mParent = this;

			triggerWidgetUpdate();
			child->triggerWidgetUpdate();
		}

		WidgetManagerInterface* getRoot();

		void triggerWidgetUpdate();

	public:
		virtual void process(const EventHandler& events) {}
		virtual void draw(Canvas& canvas) { canvas.rect(mArea.getCurrentRect(), RGBA(1.f), 10); }
		virtual void drawOverlay(Canvas& canvas) {}

		virtual void mouseEnter();
		virtual void mouseLeave();

		// size policies
		virtual void adjustRect();
		virtual void adjustChildrenRect();

		[[nodiscard]] virtual bool processesEvents() const;
		[[nodiscard]] virtual bool propagateEventsToChildren() const;

		[[nodiscard]] virtual bool needsNextFrame() const;

		virtual void endAnimations();
		virtual void updateAnimations();

	public:
		[[nodiscard]] RectF getArea() const;
		[[nodiscard]] RectF getRelativeArea() const;
		void setArea(const RectF& area);

	protected:
		Widget* mParent = nullptr;

		std::vector<Widget*> mChildren;

		// relative to the parent
		SpringRect mArea;

		ualni mZValue = 0;

		bool mInFocus = false;

		// debug
		std::string mName = "widget base";
		Vec2F mLocalPoint;
		Vec2F mGlobalPoint;
		RGBA mDebugColor = { 1, 1, 1, 0.3 };
	};

	struct WidgetManagerInterface : public Widget {
		virtual void updateWidget(Widget*) = 0;
	};
}
#pragma once

#include "SpringAnimations.hpp"
#include "LayoutPolicies.hpp"

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

		void addChild(Widget* child);
		void removeChild(Widget* child);

		WidgetManagerInterface* getRoot();

		void triggerWidgetUpdate(const char* reason = nullptr);

		void bringToFront();
		void bringToBack();

		void setSizePolicy(SizePolicy x, SizePolicy y);
		void setLayoutPolicy(LayoutPolicy layoutPolicy);

		const Vec2F& getMinSize();
		void setMinSize(const Vec2F& size);

	public:
		virtual void process(const EventHandler& events) {}
		virtual void draw(Canvas& canvas) {}
		virtual void drawOverlay(Canvas& canvas) {}

		virtual void mouseEnter();
		virtual void mouseLeave();

		// size policies
		virtual void pickRect();
		virtual void clampRect();
		virtual void adjustChildrenRect();

		// resizing
		RangeF pickRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical);
		RangeF clampRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical);

		void clampMinMaxSize();

		RectF getChildrenEnclosure() const;
		RectF getParentEnclosure();
		void adjustLayout(bool vertical);
		halnf changeChildSize(Widget*, halnf diff, bool vertical);

		[[nodiscard]] virtual bool processesEvents() const;
		[[nodiscard]] virtual bool propagateEventsToChildren() const;

		[[nodiscard]] virtual bool needsNextFrame() const;

		virtual void endAnimations();
		virtual void updateAnimations();

	public:
		[[nodiscard]] RectF getArea() const;
		[[nodiscard]] RectF getAreaT() const;
		[[nodiscard]] const RectF& getAreaCache() const;

		[[nodiscard]] RectF getRelativeArea() const;
		[[nodiscard]] RectF getRelativeAreaT() const;
		[[nodiscard]] RectF getRelativeAreaCache() const;

		void setArea(const RectF& area);
		void setAreaCache(const RectF& area);

	protected:
		friend class WidgetLayout;

		Widget* mParent = nullptr;

		std::vector<Widget*> mChildren;
		List<Widget*> mDepthOrder;

		// relative to the parent
		SpringRect mArea;

		// resizing
		LayoutPolicy mLayoutPolicy = LayoutPolicy::Passive;
		Vec2<SizePolicy> mSizePolicy = { SizePolicy::Fixed, SizePolicy::Fixed };
		Vec2F mMinSize = { 50, 50 };
		Vec2F mMaxSize = { FLT_MAX / 2, FLT_MAX / 2 };

		bool mInFocus = false;

		halnf mLayoutGap = 5;
		halnf mLayoutMargin = 10;

		// cache
		RectF mAreaCache;

		// debug
		std::string mName = "widget base";
		Vec2F mLocalPoint;
		Vec2F mGlobalPoint;
		RGBA mDebugColor = { 1, 1, 1, 0.3 };
		std::string mTriggerReason = "none";
	};

	struct WidgetManagerInterface : public Widget {
		virtual void updateWidget(Widget*, const char* reason = nullptr) = 0;
	};
}
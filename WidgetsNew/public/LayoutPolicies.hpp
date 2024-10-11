#pragma once

#include "Rect.hpp"

namespace tp {
	class Widget;

	enum class SizePolicy {
		Fixed,
		Expanding,
		Minimal,
	};

	enum class LayoutPolicy {
		Passive,
		Vertical,
		Horizontal,
	};

	class WidgetLayout {
	public:
		explicit WidgetLayout(Widget* widget) { mWidget = widget; }

		virtual void adjustWidget() = 0;

		virtual void pickRect();
		virtual void clampRect();
		virtual void adjustChildrenRect();
		void adjustLayout(bool vertical);
		halnf changeChildSize(Widget*, halnf diff, bool vertical);

		RangeF pickRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical);
		RangeF clampRange(const RangeF& current, const RangeF& children, const RangeF& parent, bool vertical);

		void clampMinMaxSize();

		RectF getChildrenEnclosure();
		RectF getParentEnclosure();

	protected:
		const RectF& getArea();
		void setArea(const RectF& area);

	private:
		Widget* mWidget = nullptr;
	};

	class BasicLayout : public WidgetLayout {
	public:
		BasicLayout() = default;

		void adjustWidget() override;

	private:


	private:
		LayoutPolicy mLayoutPolicy = LayoutPolicy::Vertical;
		Vec2<SizePolicy> mSizePolicy = { SizePolicy::Fixed, SizePolicy::Fixed };

		Vec2F mMinSize = { 50, 50 };
		Vec2F mMaxSize = { FLT_MAX / 2, FLT_MAX / 2 };

		halnf mLayoutGap = 5;
		halnf mLayoutMargin = 10;
	};
}
#pragma once

#include "Rect.hpp"
#include <vector>

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
		virtual ~WidgetLayout() = default;

		virtual void pickRect() = 0;
		virtual void clampRect() = 0;
		virtual void adjustChildrenRect() = 0;

	protected:
		[[nodiscard]] const RectF& getArea() const;
		void setArea(const RectF& area);
		[[nodiscard]] Widget* parent() const;
		[[nodiscard]] const std::vector<Widget*>& children() const;

	private:
		Widget* mWidget = nullptr;
	};

	class BasicLayout : public WidgetLayout {
	public:
		explicit BasicLayout(Widget* widget) : WidgetLayout(widget) {}

		void pickRect() override;
		void clampRect() override;
		void adjustChildrenRect() override;

	public:
		const Vec2F& getMinSize();
		void setMinSize(const Vec2F& size);

	private:
		void adjustLayout(bool vertical);
		static halnf changeChildSize(Widget*, halnf diff, bool vertical);

		[[nodiscard]] RangeF pickRange(const RangeF& current, const RangeF& child, const RangeF& parent, bool v) const;
		[[nodiscard]] RangeF clampRange(const RangeF& current, const RangeF& child, const RangeF& parent, bool v) const;

		void clampMinMaxSize();

		[[nodiscard]] RectF getChildrenEnclosure() const;
		[[nodiscard]] RectF getParentEnclosure() const;

	private:
		LayoutPolicy mLayoutPolicy = LayoutPolicy::Vertical;
		Vec2<SizePolicy> mSizePolicy = { SizePolicy::Fixed, SizePolicy::Fixed };

		Vec2F mMinSize = { 50, 50 };
		Vec2F mMaxSize = { FLT_MAX / 2, FLT_MAX / 2 };

		halnf mLayoutGap = 5;
		halnf mLayoutMargin = 10;
	};
}
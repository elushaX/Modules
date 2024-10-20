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
		friend class DebugManager;
		friend class LayoutManager;

	public:
		explicit WidgetLayout(Widget* widget) { mWidget = widget; }
		virtual ~WidgetLayout() = default;

		virtual void updateLayout(bool vertical) {}

		virtual void pickRect(bool vertical) {}
		virtual void clampRect() {}
		[[nodiscard]] virtual RectF getAvailableChildArea() const;

	public:
		const Vec2F& getMinSize();
		void setMinSize(const Vec2F& size);

		[[nodiscard]] const Vec2<SizePolicy>& getSizePolicy() const;
		void setSizePolicy(SizePolicy x, SizePolicy y);

	public:
		[[nodiscard]] const RectF& getArea() const;
		[[nodiscard]] RectF getAnimatedArea() const;

		void setArea(const RectF& area);
		[[nodiscard]] Widget* parent() const;
		[[nodiscard]] const std::vector<Widget*>& children() const;

	public:
		void clampMinMaxSize();

		[[nodiscard]] RangeF pickRange(const RangeF& current, const RangeF& child, const RangeF& parent, bool v) const;
		[[nodiscard]] RangeF clampRange(const RangeF& current, const RangeF& child, const RangeF& parent, bool v) const;

		[[nodiscard]] RectF getChildrenEnclosure() const;
		[[nodiscard]] RectF getParentEnclosure() const;

	private:
		Widget* mWidget = nullptr;

	protected:
		Vec2<SizePolicy> mSizePolicy = { SizePolicy::Fixed, SizePolicy::Fixed };
		Vec2F mMinSize = { 30, 30 };
		Vec2F mMaxSize = { FLT_MAX / 2, FLT_MAX / 2 };

	private:
		// RectF mPrevArea{};
	};
}
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

	public:
		// picks own position and size
		// modifies only own area
		virtual void pickRect(bool vertical) {}

		// arranges all children layouts inside itself
		// modifies children areas
		virtual void arrangeChildren(bool vertical) {}

		// area that any child can have
		[[nodiscard]] virtual RectF availableChildArea() const;

		// minimal size base on the content of the layout
		[[nodiscard]] virtual RectF minContentArea() const;

	public:
		const Vec2F& getMinSize();
		void setMinSize(const Vec2F& size);

		[[nodiscard]] const Vec2<SizePolicy>& getSizePolicy() const;
		void setSizePolicy(SizePolicy x, SizePolicy y);

		static WidgetLayout* lay(Widget* w);

	public:
		[[nodiscard]] const RectF& getArea() const;
		[[nodiscard]] RectF getAnimatedArea() const;

		void setArea(const RectF& area);
		[[nodiscard]] Widget* parent() const;
		[[nodiscard]] const std::vector<Widget*>& children() const;

	public:
		void clampMinMaxSize();
		void pickMinimalRect(bool dir);

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
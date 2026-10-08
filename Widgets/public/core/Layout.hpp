#pragma once

#include "Sizing.hpp"

#include <vector>

namespace tp::ui {

	class Widget;
	class LayoutEngine;

	class LayoutContext {
	public:
		LayoutContext(LayoutEngine& engine, Widget& owner);

		[[nodiscard]] const Widget& owner() const { return mOwner; }
		[[nodiscard]] const std::vector<Widget*>& children() const { return mVisibleChildren; }

		[[nodiscard]] Extent measure(const Widget& child, Axis axis, halnf crossLength) const;
		void place(Widget& child, const RectF& rect) const;
		[[nodiscard]] LayoutContext without(const Widget& child) const;

	private:
		LayoutEngine& mEngine;
		Widget& mOwner;
		std::vector<Widget*> mVisibleChildren;
	};

	class Layout {
	public:
		virtual ~Layout() = default;

		void setPadding(const Insets& padding);
		[[nodiscard]] const Insets& padding() const { return mPadding; }

		[[nodiscard]] Extent measure(const LayoutContext& context, Axis axis, halnf crossLength) const;
		void arrange(LayoutContext& context, const RectF& ownerArea);

	protected:
		[[nodiscard]] virtual Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const = 0;
		virtual void arrangeChildren(LayoutContext& context, const RectF& contentArea) = 0;

		void invalidate();

	private:
		friend class Widget;

		Widget* mOwner = nullptr;
		Insets mPadding;
	};
}

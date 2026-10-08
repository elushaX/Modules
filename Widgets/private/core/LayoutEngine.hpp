#pragma once

#include "Ui.hpp"
#include "Widget.hpp"

namespace tp::ui {

	class LayoutEngine {
	public:
		explicit LayoutEngine(Diagnostics& diagnostics) :
			mDiagnostics(diagnostics) {}

		[[nodiscard]] Extent measure(const Widget& widget, Axis axis, halnf crossLength);
		[[nodiscard]] Vec2F preferredSize(const Widget& widget);

		void layoutRoot(Widget& root, const RectF& area);
		void place(Widget& widget, const RectF& rect);

	private:
		void arrangeChildren(Widget& widget);

		Diagnostics& mDiagnostics;
	};
}

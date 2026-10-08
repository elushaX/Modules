#include "LayoutEngine.hpp"

#include "Layout.hpp"
#include "WidgetInternals.hpp"

using namespace tp;
using namespace tp::ui;

Extent LayoutEngine::measure(const Widget& widget, Axis axis, halnf crossLength) {
	Extent cached;
	if (WidgetInternals::cachedMeasure(widget, axis, crossLength, cached)) return cached;

	mDiagnostics.stats.measuredWidgets++;

	Extent content;
	if (widget.layout()) {
		LayoutContext context(*this, const_cast<Widget&>(widget));
		content = widget.layout()->measure(context, axis, crossLength);
	} else {
		content = WidgetInternals::measureContent(widget, axis, crossLength);
	}

	const Extent resolved = widget.sizing().along(axis).resolve(content);
	WidgetInternals::storeMeasure(const_cast<Widget&>(widget), axis, crossLength, resolved);
	return resolved;
}

Vec2F LayoutEngine::preferredSize(const Widget& widget) {
	const halnf width = measure(widget, Axis::Horizontal, kUnbounded).preferred;
	const halnf height = measure(widget, Axis::Vertical, width).preferred;
	return { width, height };
}

void LayoutEngine::layoutRoot(Widget& root, const RectF& area) {
	mDiagnostics.stats.layoutPasses++;
	if (mDiagnostics.observer) mDiagnostics.observer->onLayoutPassStarted();
	place(root, area);
}

void LayoutEngine::place(Widget& widget, const RectF& rect) {
	const bool rectChanged = WidgetInternals::assignTarget(widget, rect);

	if (!rectChanged && !WidgetInternals::needsLayout(widget)) {
		mDiagnostics.stats.skippedWidgets++;
		return;
	}

	mDiagnostics.stats.arrangedWidgets++;
	if (mDiagnostics.observer) mDiagnostics.observer->onWidgetArranged(widget);
	arrangeChildren(widget);
	WidgetInternals::markLaidOut(widget);
}

void LayoutEngine::arrangeChildren(Widget& widget) {
	if (!widget.layout()) return;

	LayoutContext context(*this, widget);
	widget.layout()->arrange(context, widget.localRect());
}

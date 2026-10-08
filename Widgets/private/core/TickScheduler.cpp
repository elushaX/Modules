#include "TickScheduler.hpp"

#include "WidgetInternals.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

void TickScheduler::start(Widget& widget) { insertUnique(mTicking, widget); }

void TickScheduler::stop(const Widget& widget) { erase(mTicking, widget); }

void TickScheduler::startGeometry(Widget& widget) { insertUnique(mAnimatingGeometry, widget); }

void TickScheduler::forget(const Widget& widget) {
	erase(mTicking, widget);
	erase(mAnimatingGeometry, widget);
}

void TickScheduler::advance(halnf deltaMs) {
	advanceTicks(deltaMs);
	advanceGeometry(deltaMs);
}

void TickScheduler::advanceTicks(halnf deltaMs) {
	const std::vector<Widget*> snapshot = mTicking;
	for (Widget* widget : snapshot) {
		const bool stillScheduled = std::find(mTicking.begin(), mTicking.end(), widget) != mTicking.end();
		if (stillScheduled) WidgetInternals::tick(*widget, deltaMs);
	}
}

void TickScheduler::advanceGeometry(halnf deltaMs) {
	const std::vector<Widget*> snapshot = mAnimatingGeometry;
	for (Widget* widget : snapshot) {
		const bool keepsMoving = WidgetInternals::stepGeometry(*widget, deltaMs);

		widget->requestPaint();
		if (!keepsMoving) erase(mAnimatingGeometry, *widget);
	}
}

void TickScheduler::insertUnique(std::vector<Widget*>& widgets, Widget& widget) {
	if (std::find(widgets.begin(), widgets.end(), &widget) == widgets.end()) widgets.push_back(&widget);
}

void TickScheduler::erase(std::vector<Widget*>& widgets, const Widget& widget) {
	widgets.erase(std::remove(widgets.begin(), widgets.end(), &widget), widgets.end());
}

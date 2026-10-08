#include "TaskTimeline.hpp"

#include "BoxLayout.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kGap = 6;
	constexpr const char* kLiveText = "Live board";
	constexpr const char* kNoHistoryText = "No history yet";
	constexpr const char* kWidestMoment = "0000-00-00 00:00:00";

	std::string viewingText(const std::string& moment) { return "Viewing " + moment + " (read-only)"; }
}

std::string tp::taskboard::formatMoment(double unixSeconds) {
	const std::time_t seconds = std::time_t(unixSeconds);
	std::tm local{};
	::localtime_r(&seconds, &local);

	char text[32];
	std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &local);
	return text;
}

TaskTimeline::TaskTimeline() {
	setDebugName("task timeline");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<RowLayout>(kGap).setPadding(Insets::symmetric(10, 3));

	caption.setTone(Label::Tone::Muted);
	slider.setSizing(AxisSizing::fill(), AxisSizing::fit());
	slider.setStep(1);
	moment.setSizing(AxisSizing::fit(), AxisSizing::fit());
	moment.setReservedText(viewingText(kWidestMoment));

	std::vector<std::string> labels;
	for (const TimelinePrecision& option : kTimelinePrecisions)
		labels.emplace_back(option.label);
	precisionChoice.setOptions(labels);
	precisionChoice.setSelectedIndex(long(mPrecision));

	for (Widget* child :
			 std::initializer_list<Widget*>{ &caption, &back, &slider, &forward, &moment, &precisionChoice, &live })
		addChild(*child);

	slider.setOnChanged([this](halnf value) { chooseStep(size_t(std::lround(value))); });
	back.setOnActivate([this] { previousWindow(); });
	forward.setOnActivate([this] { nextWindow(); });
	precisionChoice.setOnSelected([this](long index) { setPrecision(size_t(index)); });
	live.setOnActivate([this] { chooseLive(); });
	refresh();
}

void TaskTimeline::setSpan(std::optional<double> first, std::optional<double> last) {
	mFirst = first;
	mLast = last;
	mWindowOffset = std::min(mWindowOffset, lastWindowOffset());
	refresh();
}

void TaskTimeline::setChangeMoments(std::vector<double> moments) {
	mChanges = std::move(moments);
	refreshTicks();
}

void TaskTimeline::showMoment(std::optional<double> shown) {
	mMoment = shown;
	keepMomentInWindow();
	refresh();
}

void TaskTimeline::setPrecision(size_t index) {
	mPrecision = std::min(index, kTimelinePrecisions.size() - 1);
	precisionChoice.setSelectedIndex(long(mPrecision));
	mWindowOffset = 0;
	keepMomentInWindow();
	refresh();
}

size_t TaskTimeline::stepCount() const { return size_t(std::lround(precision().window / precision().step)); }

double TaskTimeline::windowEnd() const { return mLast.value_or(0) - double(mWindowOffset) * precision().window; }

double TaskTimeline::windowStart() const { return windowEnd() - precision().window; }

size_t TaskTimeline::lastWindowOffset() const {
	if (!hasHistory()) return 0;
	const double windows = std::ceil((*mLast - *mFirst) / precision().window);
	return windows > 1 ? size_t(windows) - 1 : 0;
}

double TaskTimeline::clampedToHistory(double shown) const {
	if (!hasHistory()) return shown;
	return std::clamp(shown, *mFirst, *mLast);
}

void TaskTimeline::keepMomentInWindow() {
	if (!mMoment || !hasHistory()) {
		if (!mMoment) mWindowOffset = 0;
		return;
	}
	const double behind = std::floor((*mLast - *mMoment) / precision().window);
	mWindowOffset = std::min(size_t(std::max(0.0, behind)), lastWindowOffset());
}

std::optional<double> TaskTimeline::momentAtStep(size_t step) const {
	if (!hasHistory()) return std::nullopt;
	if (mWindowOffset == 0 && step >= stepCount()) return std::nullopt;
	return clampedToHistory(windowStart() + double(std::min(step, stepCount())) * precision().step);
}

size_t TaskTimeline::stepOf(std::optional<double> shown) const {
	if (!shown || !hasHistory()) return stepCount();
	const double steps = std::floor((*shown - windowStart()) / precision().step + 0.5);
	return size_t(std::clamp(steps, 0.0, double(stepCount())));
}

void TaskTimeline::chooseStep(size_t step) {
	if (!hasHistory() || !mOnMomentChosen) return;
	mOnMomentChosen(momentAtStep(step));
}

void TaskTimeline::previousWindow() {
	if (!hasHistory() || mWindowOffset >= lastWindowOffset()) return;
	const double from = mMoment.value_or(*mLast);
	mWindowOffset++;
	if (mOnMomentChosen) mOnMomentChosen(clampedToHistory(from - precision().window));
	refresh();
}

void TaskTimeline::nextWindow() {
	if (!hasHistory() || mWindowOffset == 0) return;
	const double to = mMoment.value_or(*mLast) + precision().window;
	mWindowOffset--;
	if (mOnMomentChosen) mOnMomentChosen(to >= *mLast ? std::nullopt : std::optional<double>(to));
	refresh();
}

void TaskTimeline::chooseLive() {
	mWindowOffset = 0;
	if (mOnMomentChosen) mOnMomentChosen(std::nullopt);
	refresh();
}

void TaskTimeline::refresh() {
	slider.setRange(0, halnf(stepCount()));
	if (!slider.isDragging()) slider.setValue(halnf(stepOf(mMoment)));

	slider.setEnabled(hasHistory());
	back.setEnabled(hasHistory() && mWindowOffset < lastWindowOffset());
	forward.setEnabled(hasHistory() && mWindowOffset > 0);
	live.setEnabled(mMoment.has_value());
	refreshTicks();

	if (!mMoment) {
		moment.setTone(Label::Tone::Muted);
		moment.setText(hasHistory() ? kLiveText : kNoHistoryText);
		return;
	}
	moment.setTone(Label::Tone::Warning);
	moment.setText(viewingText(formatMoment(*mMoment)));
}

void TaskTimeline::refreshTicks() {
	if (!hasHistory()) {
		slider.setTickMarks({});
		return;
	}

	std::vector<halnf> fractions;
	for (double change : mChanges) {
		const double fraction = (change - windowStart()) / precision().window;
		if (fraction >= 0 && fraction <= 1) fractions.push_back(halnf(fraction));
	}
	slider.setTickMarks(std::move(fractions));
}

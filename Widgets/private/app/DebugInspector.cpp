#include "DebugInspector.hpp"

#include "Assert.hpp"
#include "Layout.hpp"
#include "Widget.hpp"

#include "imgui.h"
#include "implot.h"

#include <cstdio>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr size_t kMaximumEventRecords = 400;
	constexpr size_t kMaximumLayoutRecords = 400;
	constexpr float kMinimumPanelFraction = 0.2f;
	constexpr float kMaximumPanelFraction = 0.6f;
	constexpr float kMinimumPanelWidth = 320;

	const RGBA kOutlineColor = { 0.2f, 0.8f, 1.0f, 0.35f };
	const RGBA kHoverOutlineColor = { 1.0f, 0.75f, 0.1f, 0.9f };
	const RGBA kSelectionColor = { 1.0f, 0.25f, 0.35f, 0.95f };
	const RGBA kSelectionFill = { 1.0f, 0.25f, 0.35f, 0.12f };

	float nanosecondsToMs(time_ns nanoseconds) { return (float) ((double) nanoseconds / 1.0e6); }

	constexpr AxisSizing::Mode kSizingModes[] = {
		AxisSizing::Mode::Fit, AxisSizing::Mode::Fixed, AxisSizing::Mode::Preferred, AxisSizing::Mode::Fill
	};

	const char* sizingModeName(AxisSizing::Mode mode) {
		switch (mode) {
			case AxisSizing::Mode::Fit: return "fit";
			case AxisSizing::Mode::Fixed: return "fixed";
			case AxisSizing::Mode::Preferred: return "preferred";
			case AxisSizing::Mode::Fill: return "fill";
		}
		return "?";
	}

	void plotSeries(const char* label, const TimeSeries& series) {
		ImPlot::PlotLine(label, series.data(), series.count(), 1.0, 0.0, 0, series.offset());
	}

	void statisticsRow(const char* name, const TimeSeries& series, const char* unit) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(name);
		ImGui::TableNextColumn();
		ImGui::Text("%.3f %s", series.latest(), unit);
		ImGui::TableNextColumn();
		ImGui::Text("%.3f", series.average());
		ImGui::TableNextColumn();
		ImGui::Text("%.3f", series.maximum());
	}
}

void TimeSeries::add(float value) {
	mValues[mNext] = value;
	mNext = (mNext + 1) % kCapacity;
}

float TimeSeries::latest() const { return mValues[(mNext + kCapacity - 1) % kCapacity]; }

float TimeSeries::average() const {
	float total = 0;
	for (float value : mValues)
		total += value;
	return total / (float) kCapacity;
}

float TimeSeries::maximum() const {
	float result = 0;
	for (float value : mValues)
		result = value > result ? value : result;
	return result;
}

DebugInspector::DebugInspector(Ui& ui) :
	mUi(ui) {
	mUi.setObserver(this);
}

DebugInspector::~DebugInspector() { mUi.setObserver(nullptr); }

void DebugInspector::handleShortcuts(const EventHandler& input) {
	if (ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput) return;

	if (mToggleKey && input.isPressed(*mToggleKey)) setVisible(!mVisible);
	if (!mVisible) return;

	if (input.isPressed(InputID::K)) mFreezeProcessing = !mFreezeProcessing;

	Widget* hovered = mUi.hoveredWidget();
	if (!hovered) return;

	if (input.isPressed(InputID::S)) mSelected = hovered;
	if (input.isPressed(InputID::P)) mEventBreakpoints.insert(hovered);
	if (input.isPressed(InputID::L)) mLayoutBreakpoints.insert(hovered);
}

DebugInspector::WindowSplit DebugInspector::splitWindow(Vec2F windowSize) const {
	if (!mVisible) return { makeRect({ 0, 0 }, windowSize), makeRect(windowSize.x, 0, 0, windowSize.y) };

	float panelWidth = windowSize.x * mPanelFraction;
	if (panelWidth < kMinimumPanelWidth) panelWidth = kMinimumPanelWidth;
	if (panelWidth > windowSize.x) panelWidth = windowSize.x;

	const float contentWidth = windowSize.x - panelWidth;
	return { makeRect(0, 0, contentWidth, windowSize.y), makeRect(contentWidth, 0, panelWidth, windowSize.y) };
}

InputRouting DebugInspector::inputRouting(Vec2F pointer, Vec2F windowSize) const {
	InputRouting routing;
	if (!ImGui::GetCurrentContext()) return routing;

	const ImGuiIO& io = ImGui::GetIO();
	const bool pointerOverPanel = mVisible && contains(splitWindow(windowSize).panel, pointer);
	routing.pointerBlocked = pointerOverPanel || io.WantCaptureMouse;
	routing.keyboardBlocked = io.WantCaptureKeyboard;
	return routing;
}

void DebugInspector::recordFrame(
	halnf processMs, halnf drawMs, halnf framesProcessedPerSecond, halnf framesDrawnPerSecond
) {
	const FrameStats& stats = mUi.stats();

	mProcessMs.add(processMs);
	mDrawMs.add(drawMs);
	mInputMs.add(nanosecondsToMs(stats.inputNs - mPreviousStats.inputNs));
	mAnimationMs.add(nanosecondsToMs(stats.animationNs - mPreviousStats.animationNs));
	mLayoutMs.add(nanosecondsToMs(stats.layoutNs - mPreviousStats.layoutNs));
	mPaintMs.add(nanosecondsToMs(stats.paintNs - mPreviousStats.paintNs));
	mArranged.add((float) (stats.arrangedWidgets - mPreviousStats.arrangedWidgets));
	mMeasured.add((float) (stats.measuredWidgets - mPreviousStats.measuredWidgets));

	mFramesProcessedPerSecond = framesProcessedPerSecond;
	mFramesDrawnPerSecond = framesDrawnPerSecond;
	mPreviousStats = stats;
	mFrame++;
}

void DebugInspector::draw() {
	if (!mVisible) return;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const WindowSplit split = splitWindow({ viewport->Size.x, viewport->Size.y });

	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + split.panel.pos.x, viewport->Pos.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(split.panel.size.x, split.panel.size.y), ImGuiCond_Always);

	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
																 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (ImGui::Begin("Widget Debugger", nullptr, flags)) {
		drawPanelWidthControl();
		drawToolbar();

		if (ImGui::BeginTabBar("debugger tabs")) {
			if (ImGui::BeginTabItem("Performance")) {
				drawPerformanceTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Under Cursor")) {
				drawHoverStackTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Tree")) {
				drawWidgetTreeTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Events")) {
				drawEventsTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Layout")) {
				drawLayoutTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Popups")) {
				drawPopupsTab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void DebugInspector::paintOverlay(Painter& painter) {
	if (!mVisible) return;

	if (mShowOutlines && mUi.root()) paintOutlines(painter, *mUi.root(), 0);

	if (Widget* hovered = mUi.hoveredWidget()) painter.strokeRect(hovered->windowRect(), kHoverOutlineColor);

	if (mSelected) {
		const RectF rect = mSelected->windowRect();
		painter.fillRect(rect, kSelectionFill);
		painter.strokeRect(rect, kSelectionColor);
		painter.text(
			nameOf(*mSelected), makeRect(rect.pos.x + 4, rect.pos.y - 18, 400, 18), 12, TextAlign::Start, kSelectionColor
		);
	}
}

void DebugInspector::onEventDelivering(const Widget& receiver, const Event&) {
	if (mEventBreakpoints.erase(&receiver)) DEBUG_BREAK(true)
}

void DebugInspector::onEventDelivered(const Widget& receiver, const Event& event) {
	if (!mVisible) return;
	if (event.type() == Event::Type::PointerMove && !mLogPointerMoves) return;

	mEvents.push_front({ mFrame, eventTypeName(event), nameOf(receiver), event.windowPosition(), event.isAccepted() });
	if (mEvents.size() > kMaximumEventRecords) mEvents.pop_back();
}

void DebugInspector::onLayoutPassStarted() {
	if (!mCurrentLayoutOrder.empty()) mLastLayoutOrder.swap(mCurrentLayoutOrder);
	mCurrentLayoutOrder.clear();
}

void DebugInspector::onWidgetArranged(const Widget& widget) {
	if (mLayoutBreakpoints.erase(&widget)) DEBUG_BREAK(true)
	if (mVisible && mCurrentLayoutOrder.size() < kMaximumLayoutRecords) mCurrentLayoutOrder.push_back(nameOf(widget));
}

void DebugInspector::onWidgetDetached(const Widget& widget) {
	if (mSelected && widget.isSelfOrAncestorOf(*mSelected)) mSelected = nullptr;
	mEventBreakpoints.erase(&widget);
	mLayoutBreakpoints.erase(&widget);
}

void DebugInspector::drawPanelWidthControl() {
	ImGui::TextUnformatted("Widget Debugger");
	ImGui::SameLine();
	if (ImGui::SmallButton("Close")) mVisible = false;
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	float percent = mPanelFraction * 100;
	if (
		ImGui::SliderFloat(
			"##panel width", &percent, kMinimumPanelFraction * 100, kMaximumPanelFraction * 100, "width %.0f%%"
		)
	) {
		mPanelFraction = percent / 100;
	}
}

void DebugInspector::drawToolbar() {
	ImGui::Checkbox("Freeze [K]", &mFreezeProcessing);
	ImGui::SameLine();
	ImGui::Checkbox("Force redraw", &mForceRedraw);
	ImGui::SameLine();
	ImGui::Checkbox("Outlines", &mShowOutlines);

	ImGui::TextDisabled("Hover a widget: [S] select  [P] break on event  [L] break on layout");

	const char* state = mUi.isLayoutPending() ? "layout pending" : (mUi.needsFrame() ? "animating" : "idle");
	ImGui::Text(
		"frame %llu   %s   popups %llu", (unsigned long long) mFrame, state, (unsigned long long) mUi.openPopupCount()
	);
	ImGui::Separator();
}

void DebugInspector::drawPerformanceTab() {
	ImGui::Text("Processed %.0f fps   Drawn %.0f fps", mFramesProcessedPerSecond, mFramesDrawnPerSecond);
	ImGui::Text(
		"Input %llu received, %llu delivered",
		(unsigned long long) mWindowCounters.inputReceived,
		(unsigned long long) mWindowCounters.inputDelivered
	);
	ImGui::Text(
		"Resize %llu received, %llu applied   Swapchain rebuilds %llu",
		(unsigned long long) mWindowCounters.configuresReceived,
		(unsigned long long) mWindowCounters.configuresApplied,
		(unsigned long long) mWindowCounters.swapchainRecreations
	);

	if (ImPlot::BeginPlot("Frame time (ms)", ImVec2(-1, 200))) {
		ImPlot::SetupAxes("frame", "ms", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit);
		plotSeries("process", mProcessMs);
		plotSeries("draw", mDrawMs);
		ImPlot::EndPlot();
	}

	if (ImPlot::BeginPlot("Ui phases (ms)", ImVec2(-1, 200))) {
		ImPlot::SetupAxes("frame", "ms", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit);
		plotSeries("input", mInputMs);
		plotSeries("animation", mAnimationMs);
		plotSeries("layout", mLayoutMs);
		plotSeries("paint", mPaintMs);
		ImPlot::EndPlot();
	}

	if (ImPlot::BeginPlot("Layout work (widgets)", ImVec2(-1, 160))) {
		ImPlot::SetupAxes("frame", "widgets", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit);
		plotSeries("arranged", mArranged);
		plotSeries("measured", mMeasured);
		ImPlot::EndPlot();
	}

	if (ImGui::BeginTable("phase table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
		ImGui::TableSetupColumn("phase");
		ImGui::TableSetupColumn("last");
		ImGui::TableSetupColumn("avg");
		ImGui::TableSetupColumn("max");
		ImGui::TableHeadersRow();
		statisticsRow("process", mProcessMs, "ms");
		statisticsRow("draw", mDrawMs, "ms");
		statisticsRow("input", mInputMs, "ms");
		statisticsRow("animation", mAnimationMs, "ms");
		statisticsRow("layout", mLayoutMs, "ms");
		statisticsRow("paint", mPaintMs, "ms");
		statisticsRow("arranged", mArranged, "");
		statisticsRow("measured", mMeasured, "");
		ImGui::EndTable();
	}

	const FrameStats& stats = mUi.stats();
	ImGui::Text(
		"Totals: %llu passes, %llu arranged, %llu skipped, %llu events, %llu painted",
		(unsigned long long) stats.layoutPasses,
		(unsigned long long) stats.arrangedWidgets,
		(unsigned long long) stats.skippedWidgets,
		(unsigned long long) stats.deliveredEvents,
		(unsigned long long) stats.paintedWidgets
	);
	if (ImGui::Button("Reset totals")) {
		mUi.resetStats();
		mPreviousStats = {};
	}

	ImGui::SeparatorText("Animating");
	for (Widget* widget : mUi.animatingWidgets())
		drawWidgetLink(*widget);
}

void DebugInspector::drawHoverStackTab() {
	Widget* hovered = mUi.hoveredWidget();
	const Vec2F pointer = mUi.pointerPosition();
	ImGui::Text("pointer %.0f, %.0f", pointer.x, pointer.y);

	if (Widget* capture = mUi.pointerCapture()) ImGui::Text("pointer captured by %s", nameOf(*capture).c_str());
	if (Widget* focus = mUi.keyboardFocus()) ImGui::Text("keyboard focus: %s", nameOf(*focus).c_str());

	if (!hovered) {
		ImGui::TextDisabled("Nothing under the cursor");
		return;
	}

	ImGui::SeparatorText("Stack, innermost first");
	if (ImGui::BeginChild("hover stack", ImVec2(0, 260), ImGuiChildFlags_Border)) {
		for (Widget* widget = hovered; widget; widget = widget->parent())
			drawWidgetLink(*widget);
	}
	ImGui::EndChild();

	drawSelectedWidget();
}

void DebugInspector::drawWidgetTreeTab() {
	if (ImGui::BeginChild("widget tree", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.55f), ImGuiChildFlags_Border)) {
		if (mUi.root()) drawWidgetTreeNode(*mUi.root());
		for (Widget* popup : mUi.openPopups()) {
			ImGui::TextDisabled("popup");
			drawWidgetTreeNode(*popup);
		}
	}
	ImGui::EndChild();

	drawSelectedWidget();
}

void DebugInspector::drawWidgetTreeNode(Widget& widget) {
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (widget.children().empty()) flags |= ImGuiTreeNodeFlags_Leaf;
	if (&widget == mSelected) flags |= ImGuiTreeNodeFlags_Selected;
	if (mUi.hoveredWidget() && widget.isSelfOrAncestorOf(*mUi.hoveredWidget())) flags |= ImGuiTreeNodeFlags_DefaultOpen;

	const RectF& rect = widget.rect();
	const bool open = ImGui::TreeNodeEx(
		&widget,
		flags,
		"%s%s  [%.0f x %.0f]",
		nameOf(widget).c_str(),
		widget.isVisible() ? "" : " (hidden)",
		rect.size.x,
		rect.size.y
	);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) mSelected = &widget;

	if (!open) return;
	for (Widget* child : widget.children())
		drawWidgetTreeNode(*child);
	ImGui::TreePop();
}

void DebugInspector::drawSelectedWidget() {
	ImGui::SeparatorText("Selected");
	if (!mSelected) {
		ImGui::TextDisabled("Click a widget in a list, or hover one and press [S]");
		return;
	}

	Widget& widget = *mSelected;
	ImGui::Text("%s", nameOf(widget).c_str());

	const RectF rect = widget.rect();
	const RectF window = widget.windowRect();
	ImGui::Text("rect   %.1f, %.1f   %.1f x %.1f", rect.pos.x, rect.pos.y, rect.size.x, rect.size.y);
	ImGui::Text("window %.1f, %.1f", window.pos.x, window.pos.y);
	if (widget.isGeometryMoving()) {
		const RectF target = widget.targetRect();
		ImGui::Text("target %.1f, %.1f   %.1f x %.1f", target.pos.x, target.pos.y, target.size.x, target.size.y);
	}
	bool animated = widget.isGeometryAnimated();
	if (ImGui::Checkbox("animate geometry", &animated)) widget.setGeometryAnimated(animated);

	bool visible = widget.isVisible();
	if (ImGui::Checkbox("visible", &visible)) widget.setVisible(visible);
	ImGui::SameLine();
	bool enabled = widget.isEnabled();
	if (ImGui::Checkbox("enabled", &enabled)) widget.setEnabled(enabled);
	ImGui::SameLine();
	ImGui::Text("%s%s", widget.isHovered() ? "hovered " : "", widget.hasKeyboardFocus() ? "focused" : "");

	drawSizingEditor(widget, Axis::Horizontal);
	drawSizingEditor(widget, Axis::Vertical);

	if (Layout* layout = widget.layout()) {
		Insets padding = layout->padding();
		if (ImGui::DragFloat4("padding", &padding.left, 0.5f, 0, 200)) layout->setPadding(padding);
	}

	if (!widget.tooltip().empty()) ImGui::Text("tooltip: %s", widget.tooltip().c_str());

	if (ImGui::Button("Select parent") && widget.parent()) mSelected = widget.parent();
	ImGui::SameLine();
	if (ImGui::Button("Break on event")) mEventBreakpoints.insert(&widget);
	ImGui::SameLine();
	if (ImGui::Button("Break on layout")) mLayoutBreakpoints.insert(&widget);
	ImGui::SameLine();
	if (ImGui::Button("Relayout")) widget.requestLayout();
}

void DebugInspector::drawEventsTab() {
	ImGui::Checkbox("Log pointer moves", &mLogPointerMoves);
	ImGui::SameLine();
	if (ImGui::Button("Clear")) mEvents.clear();

	const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV;
	if (!ImGui::BeginTable("events", 5, flags, ImVec2(0, ImGui::GetContentRegionAvail().y))) return;

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("frame");
	ImGui::TableSetupColumn("event");
	ImGui::TableSetupColumn("receiver");
	ImGui::TableSetupColumn("at");
	ImGui::TableSetupColumn("accepted");
	ImGui::TableHeadersRow();

	for (const EventRecord& record : mEvents) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%llu", (unsigned long long) record.frame);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(record.type.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(record.receiver.c_str());
		ImGui::TableNextColumn();
		ImGui::Text("%.0f, %.0f", record.position.x, record.position.y);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(record.accepted ? "yes" : "");
	}
	ImGui::EndTable();
}

void DebugInspector::drawLayoutTab() {
	ImGui::Text("Last layout pass arranged %llu widgets, in order:", (unsigned long long) mLastLayoutOrder.size());
	if (ImGui::BeginChild("layout order", ImVec2(0, 0), ImGuiChildFlags_Border)) {
		for (size_t index = 0; index < mLastLayoutOrder.size(); index++) {
			ImGui::Text("%3llu  %s", (unsigned long long) index, mLastLayoutOrder[index].c_str());
		}
	}
	ImGui::EndChild();
}

void DebugInspector::drawPopupsTab() {
	const std::vector<Widget*> popups = mUi.openPopups();
	if (popups.empty()) ImGui::TextDisabled("No popups open");

	for (size_t index = 0; index < popups.size(); index++) {
		ImGui::Text("%llu.", (unsigned long long) index);
		ImGui::SameLine();
		drawWidgetLink(*popups[index]);
	}

	if (!popups.empty() && ImGui::Button("Close all")) mUi.closeAllPopups();
	if (Widget* tooltip = mUi.openTooltip()) ImGui::Text("tooltip open: %s", nameOf(*tooltip).c_str());
}

AxisSizing
DebugInspector::editedSizing(AxisSizing::Mode mode, halnf length, halnf weight, halnf minimum, halnf maximum) {
	AxisSizing sizing = AxisSizing::fit();
	switch (mode) {
		case AxisSizing::Mode::Fit: sizing = AxisSizing::fit(); break;
		case AxisSizing::Mode::Fixed: sizing = AxisSizing::fixed(length); break;
		case AxisSizing::Mode::Preferred: sizing = AxisSizing::preferred(length); break;
		case AxisSizing::Mode::Fill: sizing = AxisSizing::fill(weight > 0 ? weight : 1); break;
	}

	sizing = sizing.atLeast(minimum);
	if (maximum >= 0) sizing = sizing.atMost(maximum);
	return sizing;
}

void DebugInspector::drawSizingEditor(Widget& widget, Axis axis) {
	const AxisSizing current = widget.sizing().along(axis);
	const char* axisName = axis == Axis::Horizontal ? "width" : "height";
	const halnf placedLength = lengthAlong(widget.rect(), axis);

	ImGui::PushID(axisName);

	int modeIndex = 0;
	for (int index = 0; index < 4; index++) {
		if (kSizingModes[index] == current.mode()) modeIndex = index;
	}

	AxisSizing::Mode mode = current.mode();
	halnf length = current.fixedLength() > 0 ? current.fixedLength() : placedLength;
	halnf weight = current.fillWeight();
	halnf minimum = current.minimum();
	halnf maximum = current.maximum() >= kUnbounded ? -1.0f : current.maximum();
	bool changed = false;

	ImGui::SetNextItemWidth(100);
	if (ImGui::Combo(axisName, &modeIndex, "fit\0fixed\0preferred\0fill\0")) {
		mode = kSizingModes[modeIndex];
		changed = true;
	}

	const bool usesLength = mode == AxisSizing::Mode::Fixed || mode == AxisSizing::Mode::Preferred;
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	if (usesLength) changed |= ImGui::DragFloat("len", &length, 1.0f, 0.0f, 10000.0f, "%.0f");
	else if (mode == AxisSizing::Mode::Fill) changed |= ImGui::DragFloat("weight", &weight, 0.05f, 0.05f, 20.0f, "%.2f");
	else ImGui::TextDisabled("len %.0f", placedLength);

	ImGui::SetNextItemWidth(70);
	changed |= ImGui::DragFloat("min", &minimum, 1.0f, 0.0f, 10000.0f, "%.0f");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70);
	changed |= ImGui::DragFloat("max (-1 none)", &maximum, 1.0f, -1.0f, 10000.0f, "%.0f");

	if (changed) widget.setSizing(axis, DebugInspector::editedSizing(mode, length, weight, minimum, maximum));

	ImGui::PopID();
}

void DebugInspector::drawWidgetLink(Widget& widget) {
	ImGui::PushID(&widget);
	const RectF rect = widget.windowRect();
	char label[256];
	std::snprintf(
		label,
		sizeof(label),
		"%s  [%.0f, %.0f  %.0f x %.0f]",
		nameOf(widget).c_str(),
		rect.pos.x,
		rect.pos.y,
		rect.size.x,
		rect.size.y
	);
	if (ImGui::Selectable(label, &widget == mSelected)) mSelected = &widget;
	ImGui::PopID();
}

void DebugInspector::paintOutlines(Painter& painter, const Widget& widget, int depth) {
	if (!widget.isVisible()) return;

	painter.strokeRect(widget.windowRect(), kOutlineColor);
	for (const Widget* child : widget.children())
		paintOutlines(painter, *child, depth + 1);
}

std::string DebugInspector::nameOf(const Widget& widget) {
	return widget.debugName().empty() ? std::string("widget") : widget.debugName();
}

const char* DebugInspector::eventTypeName(const Event& event) {
	switch (event.type()) {
		case Event::Type::PointerMove: return "move";
		case Event::Type::PointerDown: return "down";
		case Event::Type::PointerUp: return "up";
		case Event::Type::Click: return "click";
		case Event::Type::LongPress: return "long press";
		case Event::Type::Wheel: return "wheel";
		case Event::Type::KeyDown: return "key down";
		case Event::Type::KeyUp: return "key up";
		case Event::Type::TextInput: return "text";
		case Event::Type::Gesture: return "gesture";
	}
	return "?";
}

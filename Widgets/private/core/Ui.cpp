#include "Ui.hpp"

#include "InputRouter.hpp"
#include "LayoutEngine.hpp"
#include "Painter.hpp"
#include "PopupStack.hpp"
#include "TickScheduler.hpp"
#include "TooltipController.hpp"
#include "WidgetInternals.hpp"

#include <algorithm>
#include <sstream>

using namespace tp;
using namespace tp::ui;

namespace {
	class ApproximateTextMetrics : public TextMetrics {
	public:
		[[nodiscard]] halnf styledTextWidth(const std::string& text, const TextStyle& style) const override {
			const halnf boldFactor = style.face == FontFace::Bold || style.face == FontFace::BoldItalic ? 1.08f : 1.0f;
			const halnf monospaceFactor = style.face == FontFace::Monospace ? 1.1f : 1.0f;
			return (halnf) text.size() * style.fontSize * 0.55f * boldFactor * monospaceFactor;
		}
	};

	const ApproximateTextMetrics gApproximateTextMetrics;

	void dumpSubtree(const Widget& widget, int depth, std::ostringstream& out) {
		const RectF& rect = widget.rect();
		out << std::string((size_t) depth * 2, ' ') << (widget.debugName().empty() ? "widget" : widget.debugName()) << " ["
				<< rect.pos.x << ", " << rect.pos.y << ", " << rect.size.x << " x " << rect.size.y << "]";
		if (!widget.isVisible()) out << " hidden";
		if (!widget.isEnabled()) out << " disabled";
		if (widget.isHovered()) out << " hovered";
		out << "\n";

		for (const Widget* child : widget.children())
			dumpSubtree(*child, depth + 1, out);
	}
}

Ui::Ui(const Theme& theme) :
	mTheme(theme),
	mLayoutEngine(std::make_unique<LayoutEngine>(mDiagnostics)),
	mPopups(std::make_unique<PopupStack>([this](const PopupRequest& request) { onPopupClosed(request); })),
	mTicks(std::make_unique<TickScheduler>()) {
	mTooltips = std::make_unique<TooltipController>(*this);
	mInput = std::make_unique<InputRouter>(*this, *mPopups, *mTooltips, mDiagnostics);
}

Ui::~Ui() {
	mPopups->closeAll();
	setRoot(nullptr);
}

void Ui::setRoot(Widget* root) {
	if (root == mRoot) return;

	if (mRoot) WidgetInternals::detach(*mRoot);
	mRoot = root;

	if (mRoot) {
		if (mRoot->parent()) mRoot->parent()->removeChild(*mRoot);
		WidgetInternals::attach(*mRoot, *this);
		mRoot->requestLayout();
	}

	scheduleLayout();
	schedulePaint();
}

void Ui::setViewport(const RectF& viewport) {
	if (viewport == mViewport) return;

	mViewport = viewport;
	scheduleLayout();
}

void Ui::resizeViewport(const RectF& viewport) {
	if (viewport == mViewport) return;

	mViewport = viewport;
	mViewportResizing = true;
	scheduleLayout();
	ensureLayout();
	mViewportResizing = false;
}

void Ui::setInteractionMode(InteractionMode mode) {
	if (mInteractionMode == mode) return;
	mInteractionMode = mode;
	closeAllPopups();
	if (mRoot) {
		WidgetInternals::notifyInteractionModeChanged(*mRoot);
		mRoot->requestLayout();
	}
	schedulePaint();
}

void Ui::setTheme(const Theme& theme) {
	mTheme = theme;

	if (mRoot) {
		WidgetInternals::notifyThemeChanged(*mRoot);
		mRoot->requestLayout();
	}
	for (Widget* popup : openPopups())
		WidgetInternals::notifyThemeChanged(*popup);

	schedulePaint();
}

void Ui::setTextMetrics(const TextMetrics* metrics) {
	mTextMetrics = metrics;
	if (mRoot) mRoot->requestLayout();
}

const TextMetrics& Ui::textMetrics() const { return mTextMetrics ? *mTextMetrics : fallbackTextMetrics(); }

const TextMetrics& Ui::fallbackTextMetrics() { return gApproximateTextMetrics; }

void Ui::dispatch(Event event) {
	ensureLayout();
	{
		ScopedNanoseconds timer(mDiagnostics.stats.inputNs);
		mInput->dispatch(event);
		if (mOnEventDispatched) mOnEventDispatched();
	}
	ensureLayout();
}

void Ui::advance(halnf deltaMs) {
	mInput->advanceClock(deltaMs > 0 ? deltaMs : 0);
	deltaMs = deltaMs < kMaximumFrameDeltaMs ? deltaMs : kMaximumFrameDeltaMs;
	{
		ScopedNanoseconds timer(mDiagnostics.stats.animationNs);
		if (mOnAdvance) mOnAdvance(deltaMs);
		mTicks->advance(deltaMs);
		mTooltips->advance(deltaMs);
	}
	ensureLayout();
}

void Ui::updateLayout() { ensureLayout(); }

void Ui::paint(Painter& painter) {
	ensureLayout();
	ScopedNanoseconds timer(mDiagnostics.stats.paintNs);

	ScopedClip viewportClip(painter, mViewport);
	if (mRoot) paintSubtree(painter, *mRoot, mViewport);

	for (auto& entry : mPopups->entries()) {
		Widget& content = *entry.request.content;
		if (entry.request.options.modal) painter.fillRect(mViewport, theme().palette.scrim);
		ScopedClip popupClip(painter, content.rect());
		paintSubtree(painter, content, content.rect());
	}

	mPaintPending = false;
}

bool Ui::needsFrame() const {
	return mLayoutPending || mPaintPending || mTicks->isActive() || mTooltips->isWaiting() ||
				 mInput->isWaitingForLongPress();
}

void Ui::openPopup(const PopupRequest& request) {
	if (!request.content) return;
	if (!mPopups->open(request)) return;
	WidgetInternals::resetPlacement(*request.content);
	WidgetInternals::attach(*request.content, *this);
	request.content->appearFrom(popupOrigin(request));
	request.content->requestLayout();
	if (request.owner) WidgetInternals::notifyOwnedPopupChanged(*request.owner);
	scheduleLayout();
	schedulePaint();
}

RectF Ui::popupOrigin(const PopupRequest& request) const {
	if (request.options.appearFrom) return *request.options.appearFrom;
	const RectF& anchor = request.anchor;

	switch (request.options.placement) {
		case Placement::Below: return makeRect(anchor.pos.x, anchor.pos.y + anchor.size.y, anchor.size.x, 0);
		case Placement::Above: return makeRect(anchor.pos.x, anchor.pos.y, anchor.size.x, 0);
		case Placement::Right: return makeRect(anchor.pos.x + anchor.size.x, anchor.pos.y, 0, anchor.size.y);
		case Placement::Left: return makeRect(anchor.pos.x, anchor.pos.y, 0, anchor.size.y);
		case Placement::Over: return anchor;
	}
	return anchor;
}

void Ui::closePopup(const Widget& content) {
	if (!mPopups->isOpen(content)) return;

	mPopups->close(content);
	mInput->refreshHover();
	schedulePaint();
}

void Ui::closePopupChain(const Widget& content) {
	const long chainRoot = mPopups->chainRootOf(content);
	if (chainRoot < 0) return;

	mPopups->closeFrom((size_t) chainRoot);
	mInput->refreshHover();
	schedulePaint();
}

void Ui::closeAllPopups() {
	mPopups->closeAll();
	mInput->refreshHover();
	schedulePaint();
}

bool Ui::isPopupOpen(const Widget& content) const { return mPopups->isOpen(content); }

const RectF* Ui::popupAnchorOf(const Widget& content) const {
	const long index = mPopups->indexOf(content);
	if (index < 0) return nullptr;
	return &mPopups->entries()[(size_t) index].request.anchor;
}

ualni Ui::openPopupCount() const { return mPopups->entries().size(); }

void Ui::setWindowHost(WindowHost* host) {
	if (mWindowHost == host) return;

	mWindowHost = host;
	if (mRoot) WidgetInternals::notifyWindowHostChanged(*mRoot);
	for (Widget* popup : openPopups())
		WidgetInternals::notifyWindowHostChanged(*popup);
	mInput->refreshCursor();
}

void Ui::refreshCursor() { mInput->refreshCursor(); }

void Ui::capturePointer(Widget& widget) { mInput->capture(widget); }

void Ui::releasePointer(const Widget& widget) { mInput->release(widget); }

Widget* Ui::pointerCapture() const { return mInput->captured(); }

void Ui::setKeyboardFocus(Widget* widget) { mInput->setKeyboardFocus(widget); }

Widget* Ui::keyboardFocus() const { return mInput->keyboardFocus(); }

bool Ui::isKeyboardNavigating() const { return mInput->isKeyboardNavigating(); }

void Ui::setActiveWidget(Widget* widget) { mInput->setActiveWidget(widget); }

Widget* Ui::activeWidget() const { return mInput->activeWidget(); }

Widget* Ui::hoveredWidget() const { return mInput->hovered(); }

Vec2F Ui::pointerPosition() const { return mInput->pointer(); }

bool Ui::isPrimaryButtonDown() const { return mInput->isPrimaryDown(); }

bool Ui::isKeyDown(InputID key) const { return mInput->isKeyDown(key); }

Widget* Ui::widgetAt(Vec2F windowPoint) const { return mInput->widgetAt(windowPoint); }

Widget* Ui::openTooltip() const { return mTooltips->bubble(); }

std::vector<Widget*> Ui::animatingWidgets() const {
	std::vector<Widget*> widgets = mTicks->ticking();
	for (Widget* widget : mTicks->animatingGeometry()) {
		if (std::find(widgets.begin(), widgets.end(), widget) == widgets.end()) widgets.push_back(widget);
	}
	return widgets;
}

std::vector<Widget*> Ui::openPopups() const {
	std::vector<Widget*> contents;
	for (const auto& entry : mPopups->entries())
		contents.push_back(entry.request.content);
	return contents;
}

std::string Ui::dumpTree() const {
	std::ostringstream out;
	if (mRoot) dumpSubtree(*mRoot, 0, out);
	for (const auto& entry : mPopups->entries()) {
		out << "popup:\n";
		dumpSubtree(*entry.request.content, 1, out);
	}
	return out.str();
}

void Ui::startTicking(Widget& widget) {
	mTicks->start(widget);
	schedulePaint();
}

void Ui::stopTicking(const Widget& widget) { mTicks->stop(widget); }

void Ui::startGeometryAnimation(Widget& widget) { mTicks->startGeometry(widget); }

void Ui::onPopupClosed(const PopupRequest& request) {
	WidgetInternals::detach(*request.content);
	if (request.owner && request.owner->ui() == this) WidgetInternals::notifyOwnedPopupChanged(*request.owner);
	schedulePaint();
}

void Ui::onWidgetHidden(Widget& widget) {
	mPopups->closeOwnedBy(widget);
	mInput->forget(widget);
	mTooltips->forget(widget);
}

void Ui::onWidgetDetached(Widget& widget) {
	onWidgetHidden(widget);
	mTicks->forget(widget);
	if (mDiagnostics.observer) mDiagnostics.observer->onWidgetDetached(widget);
}

void Ui::ensureLayout() {
	if (!mLayoutPending) return;
	mLayoutPending = false;
	ScopedNanoseconds timer(mDiagnostics.stats.layoutNs);

	if (mRoot) mLayoutEngine->layoutRoot(*mRoot, mViewport);
	layoutPopups();

	mInput->refreshHover();
	schedulePaint();
}

void Ui::layoutPopups() {
	for (auto& entry : mPopups->entries()) {
		const PopupRequest& request = entry.request;
		const Vec2F preferred = mLayoutEngine->preferredSize(*request.content);
		const Vec2F size = { std::min(preferred.x, mViewport.size.x), std::min(preferred.y, mViewport.size.y) };

		entry.rect = placePopup(request.anchor, size, mViewport, request.options);
		mLayoutEngine->place(*request.content, entry.rect);
	}
}

namespace {
	constexpr halnf kPaintOverhang = 8;
}

void Ui::paintChildren(Painter& painter, Widget& widget, const RectF& visibleArea) {
	const Widget* raised = WidgetInternals::raisedChild(widget);
	Widget* raisedChild = nullptr;

	for (Widget* child : widget.children()) {
		if (child == raised) {
			raisedChild = child;
			continue;
		}
		paintSubtree(painter, *child, visibleArea);
	}

	if (!raisedChild) return;
	WidgetInternals::paintRaisedBackdrop(widget, painter, raisedChild->rect());
	paintSubtree(painter, *raisedChild, visibleArea);
}

void Ui::paintSubtree(Painter& painter, Widget& widget, const RectF& visibleArea) {
	if (!widget.isVisible()) return;

	const RectF& rect = widget.rect();
	if (rect.size.x <= 0 || rect.size.y <= 0) return;
	if (!overlaps(insetRect(widget.windowRect(), -kPaintOverhang), visibleArea)) return;

	mDiagnostics.stats.paintedWidgets++;

	ScopedTranslation translation(painter, rect.pos);
	WidgetInternals::paint(widget, painter);

	if (widget.clipsChildren()) {
		const RectF clipped = intersection(visibleArea, widget.windowRect());
		ScopedClip clip(painter, makeRect({ 0, 0 }, rect.size));
		paintChildren(painter, widget, clipped);
	} else {
		paintChildren(painter, widget, visibleArea);
	}

	WidgetInternals::paintOverlay(widget, painter);
}

#include "TreeView.hpp"

#include "BoxLayout.hpp"
#include "Event.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace tp::ui {
	class TreeViewLayout : public Layout {
	public:
		explicit TreeViewLayout(TreeView& view) :
			mView(view) {}

	protected:
		[[nodiscard]] Extent measureChildren(const LayoutContext&, Axis, halnf) const override {
			return { 0, 0, kUnbounded, 1 };
		}
		void arrangeChildren(LayoutContext& context, const RectF& area) override { mView.arrangeCards(context, area); }

	private:
		TreeView& mView;
	};
}

namespace {
	constexpr int kCurveSegments = 16;
	constexpr halnf kLabelSize = 20;
	constexpr halnf kMinimumLabelSize = 7;
	constexpr halnf kContentPadding = 8;
	constexpr halnf kHeaderHeight = 24;
	constexpr halnf kCardRadius = 8;
	constexpr halnf kStackOffset = 4;
	constexpr halnf kContentShownDetail = 0.6f;
	constexpr halnf kFullDecorationHeight = 60;
	constexpr halnf kPortsHiddenHeight = 18;
	constexpr halnf kPortsShownHeight = 36;
	constexpr halnf kCountShownHeight = 30;
	constexpr halnf kPlainAccentWidth = 4;
	constexpr halnf kPlainAccentInset = 5;

	halnf decorationScale(halnf cardHeight) { return std::clamp(cardHeight / kFullDecorationHeight, 0.0f, 1.0f); }

	halnf portVisibility(halnf cardHeight) {
		return std::clamp((cardHeight - kPortsHiddenHeight) / (kPortsShownHeight - kPortsHiddenHeight), 0.0f, 1.0f);
	}

	halnf length(Vec2F vector) { return std::sqrt(vector.x * vector.x + vector.y * vector.y); }

	Vec2F mix(Vec2F from, Vec2F to, halnf t) { return from + (to - from) * t; }

	RGBA withAlpha(RGBA color, halnf alpha) {
		color.a *= alpha;
		return color;
	}

	void paintFitted(
		Painter& painter, const std::string& text, const RectF& box, halnf size, TextAlign align, const RGBA& color
	) {
		if (text.empty() || box.size.x <= 0 || box.size.y <= 0) return;
		halnf fontSize = std::min(size, box.size.y * 0.7f);
		const halnf width = painter.textWidth(text, fontSize);
		if (width > box.size.x && width > 0) fontSize *= box.size.x / width;
		if (fontSize < kMinimumLabelSize) return;
		painter.text(text, box, fontSize, align, color);
	}

	RectF boundsOf(const std::vector<Vec2F>& points) {
		Vec2F low = points.front();
		Vec2F high = points.front();
		for (Vec2F point : points) {
			low = { std::min(low.x, point.x), std::min(low.y, point.y) };
			high = { std::max(high.x, point.x), std::max(high.y, point.y) };
		}
		return makeRect(low, high - low);
	}
}

TreeCard::TreeCard(TreeItemId id) :
	mId(id) {
	setDebugName("tree card #" + std::to_string(id));
	setClipsChildren(true);
	setGeometryAnimated(false);
	emplaceLayout<ColumnLayout>().setPadding(Insets{ 0, kHeaderHeight, 0, 0 });
	mContent.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mContent.emplaceLayout<ColumnLayout>(4).setPadding(Insets::uniform(kContentPadding));
	addChild(mContent);
}

void TreeCard::setTitle(std::string title) {
	if (title == mTitle) return;
	mTitle = std::move(title);
	requestPaint();
}

void TreeCard::setLabel(std::string label) {
	if (label == mLabel) return;
	mLabel = std::move(label);
	requestPaint();
}

void TreeCard::setAccent(std::optional<RGBA> accent) {
	mAccent = accent;
	requestPaint();
}

void TreeCard::setBadge(std::optional<RGBA> color, bool pulsing) {
	mBadge = color;
	mBadgePulsing = color && pulsing;
	if (mBadgePulsing) startTicking();
	requestPaint();
}

void TreeCard::setHeaderShown(bool shown) {
	if (shown == mHeaderShown) return;
	mHeaderShown = shown;
	const halnf accentSpace = shown ? 0 : kPlainAccentInset + kPlainAccentWidth;
	layout()->setPadding(Insets{ accentSpace, shown ? kHeaderHeight : 0, 0, 0 });
	requestPaint();
}

RGBA TreeCard::accent() const { return mAccent.value_or(theme().palette.accent); }

void TreeCard::onTick(halnf deltaMs) {
	mClockMs += deltaMs;
	requestPaint();
	if (!mBadgePulsing) stopTicking();
}

void TreeCard::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF rect = localRect();
	const halnf radius = std::min(kCardRadius, rect.size.y / 2);

	if (mHeaderShown) paintHeaderedBody(painter, rect, radius);
	else paintPlainBody(painter, rect, radius);

	if (mDetail < 0.999f) {
		paintFitted(
			painter,
			mLabel,
			insetRect(rect, 6),
			kLabelSize,
			TextAlign::Center,
			withAlpha(style.palette.onAccent, (1 - mDetail) * mOpacity)
		);
	}

	const halnf badgeOpacity = mHeaderShown ? 1.0f : 1 - mDetail;
	if (mBadge && badgeOpacity > 0.001f) {
		const halnf header = std::min(kHeaderHeight, rect.size.y);
		const halnf scale = decorationScale(rect.size.y);
		const halnf pulse = mBadgePulsing ? 0.55f + 0.45f * std::sin(mClockMs / 260.0f) : 1.0f;
		const halnf dotRadius = 4 * scale;
		const Vec2F dot = { rect.size.x - 11 * scale, mDetail > 0.5f ? header / 2 : 10 * scale };
		if (dotRadius >= 1) painter.fillCircle(dot, dotRadius, withAlpha(*mBadge, pulse * badgeOpacity * mOpacity));
	}
}

void TreeCard::paintHeaderedBody(Painter& painter, const RectF& rect, halnf radius) {
	const Theme& style = theme();
	const halnf header = std::min(kHeaderHeight, rect.size.y);

	painter.fillRect(rect, withAlpha(accent(), mOpacity), radius);
	if (mDetail <= 0.001f) return;

	const RectF body = makeRect(0, header, rect.size.x, rect.size.y - header);
	painter.fillRect(body, withAlpha(style.palette.raisedSurface, mDetail * mOpacity), radius);
	const halnf badgeSpace = mBadge ? 16.0f : 0.0f;
	const RectF titleBox = makeRect(10, 0, rect.size.x - 20 - badgeSpace, header);
	paintFitted(
		painter, mTitle, titleBox, style.metrics.fontSize, TextAlign::Start, withAlpha(style.palette.onAccent, mDetail * mOpacity)
	);
}

void TreeCard::paintPlainBody(Painter& painter, const RectF& rect, halnf radius) {
	const Theme& style = theme();

	painter.fillRect(rect, withAlpha(Palette::mix(accent(), style.palette.raisedSurface, mDetail), mOpacity), radius);
	if (mDetail <= 0.001f) return;

	const halnf inset = std::min(kPlainAccentInset, rect.size.y / 4);
	const RectF strip = makeRect(inset, inset, kPlainAccentWidth, std::max(0.0f, rect.size.y - inset * 2));
	painter.fillRect(strip, withAlpha(accent(), mDetail * mOpacity), kPlainAccentWidth / 2);
}

void TreeCard::paintOverlay(Painter& painter) {
	const Theme& style = theme();
	const RectF rect = localRect();
	const halnf radius = std::min(kCardRadius, rect.size.y / 2);
	const halnf scale = decorationScale(rect.size.y);
	if (mHidden > 0 && mStack > 0.001f && rect.size.y >= kCountShownHeight) {
		const RectF corner = makeRect(0, rect.size.y - 16, rect.size.x - 7, 14);
		painter.text(
			"+" + std::to_string(mHidden),
			corner,
			style.metrics.smallFontSize - 1,
			TextAlign::End,
			withAlpha(style.palette.onAccent, mStack * mOpacity * (1 - mDetail * 0.4f))
		);
	}
	const halnf stroke = std::max(1.0f, 2.0f * scale);
	if (mSelected) {
		painter.strokeRect(rect, withAlpha(style.palette.accent, mOpacity), radius, mCurrent ? stroke * 1.25f : stroke);
		const halnf ring = 3 * scale;
		if (mCurrent && ring >= 1)
			painter.strokeRect(insetRect(rect, -ring), withAlpha(style.palette.accent, 0.35f * mOpacity), radius + ring, 1);
	} else if (mHover > 0.001f)
		painter.strokeRect(rect, withAlpha(style.palette.text, mHover * 0.7f * mOpacity), radius, stroke * 0.75f);
}

TreeView::TreeView(const TreeModel& model) :
	mModel(model) {
	setDebugName("tree view");
	setClipsChildren(true);
	setLayout(std::make_unique<TreeViewLayout>(*this));
	mFrame.setOnBack([this] { leaveFullView(); });
	addChild(mFrame);
	modelChanged();
}

TreeView::~TreeView() {
	for (auto& [id, card] : mCards)
		removeChild(*card);
	removeChild(mFrame);
}

void TreeView::setCardFactory(CardFactory factory) {
	mFactory = std::move(factory);
	for (auto& [id, card] : mCards)
		removeChild(*card);
	mCards.clear();
	syncCards();
	requestLayout();
}

void TreeView::setCardUpdater(CardUpdater updater) {
	mUpdater = std::move(updater);
	refreshCards();
}

void TreeView::refreshCards() {
	if (!mUpdater) return;
	for (auto& [id, card] : mCards)
		mUpdater(*card);
	requestPaint();
}

void TreeView::modelChanged() {
	const bool first = mStructure.empty();
	mStructure.rebuild(mModel);
	mExpansion.retain(mStructure);
	if (first) mExpansion.openTo(mStructure, std::nullopt, mStructure.height());

	std::erase_if(mSelected, [this](TreeItemId id) { return !mStructure.indexOf(id); });
	if (mSelection && !mSelected.contains(*mSelection)) mSelection.reset();
	if (mHovered && !mStructure.indexOf(*mHovered)) mHovered.reset();
	if (mTugged && !mStructure.indexOf(*mTugged)) mTugged.reset();
	if (mPivot && !mStructure.indexOf(*mPivot)) mPivot.reset();
	if (mLink && !mStructure.indexOf(mLink->from.item)) mLink.reset();
	if (mFullPull && !mStructure.indexOf(*mFullPull)) {
		mFullPull.reset();
		mFullStep.stop();
	}
	if (mFullSettling && !mStructure.indexOf(*mFullSettling)) mFullSettling.reset();
	if (mFull && !mStructure.indexOf(*mFull)) leaveFullView(false);
	std::erase_if(mHover, [this](const auto& entry) { return !mStructure.indexOf(entry.first); });
	mCandidate.reset();
	mLevelStep.stop();

	syncCards();
	repack();
	syncItems(!ui());
	if (mSelection) pivotOn(mSelection, size() / 2.0f, true);
	viewChanged();
}

void TreeView::syncCards() {
	for (auto entry = mCards.begin(); entry != mCards.end();) {
		if (mStructure.indexOf(entry->first)) {
			++entry;
			continue;
		}
		removeChild(*entry->second);
		entry = mCards.erase(entry);
	}
	if (!mFactory) return;
	for (const TreeStructure::Item& item : mStructure.items()) {
		if (mCards.contains(item.id)) continue;
		std::unique_ptr<TreeCard> created = mFactory(item.id);
		if (!created) continue;
		if (mUpdater) mUpdater(*created);
		addChild(*created);
		mCards.emplace(item.id, std::move(created));
	}
}

void TreeView::repack() {
	mCurrent = TreePacking::pack(mStructure, mExpansion, mStyle.layout);
	if (mCandidate) mNext = TreePacking::pack(mStructure, mCandidate->expansion, mStyle.layout);
	else mNext = mCurrent;
}

void TreeView::syncItems(bool jump) {
	const std::vector<TreeStructure::Item>& items = mStructure.items();
	std::erase_if(mPositions, [this](const auto& entry) { return !mStructure.indexOf(entry.first); });
	std::erase_if(mOpacity, [this](const auto& entry) { return !mStructure.indexOf(entry.first); });
	for (size_t item = 0; item < items.size(); item++) {
		const TreeItemId id = items[item].id;
		const Vec2F target = mCurrent.centers[item];
		const halnf opacity = isShownIndex(item) ? 1.0f : 0.0f;
		auto position = mPositions.find(id);
		if (position == mPositions.end()) {
			Vec2F start = target;
			const size_t parent = items[item].parent;
			if (!jump && parent != TreeStructure::kNone) {
				const auto parentPosition = mPositions.find(items[parent].id);
				if (parentPosition != mPositions.end()) start = parentPosition->second.value();
			}
			position = mPositions.emplace(id, AnimatedVec2(start, Motion::smooth(), 0.1f)).first;
		}
		auto fade = mOpacity.find(id);
		if (fade == mOpacity.end()) {
			const bool appearing = !jump && items[item].parent != TreeStructure::kNone;
			fade = mOpacity.emplace(id, AnimatedFloat(appearing ? 0.0f : opacity)).first;
		}
		if (jump) {
			position->second.jumpTo(target);
			fade->second.jumpTo(opacity);
		} else {
			position->second.animateTo(target);
			fade->second.animateTo(opacity);
		}
	}
	startTicking();
}

void TreeView::retarget(bool jump) {
	repack();
	syncItems(jump);
	viewChanged();
}

halnf TreeView::displayedOpacity(size_t item) const {
	const auto fade = mOpacity.find(mStructure.items()[item].id);
	halnf opacity = fade == mOpacity.end() ? (isShownIndex(item) ? 1.0f : 0.0f) : fade->second.value();
	if (const halnf preview = levelPreview(); preview > 0) {
		const halnf next = mCandidate->expansion.isShown(mStructure, item) ? 1.0f : 0.0f;
		opacity = opacity + (next - opacity) * preview;
	}
	if (morphingItem()) opacity *= backdropOpacity();
	return std::clamp(opacity, 0.0f, 1.0f);
}

halnf TreeView::backdropOpacity() const { return 1 - std::clamp(mFrame.progress(), 0.0f, 1.0f); }

std::optional<TreeItemId> TreeView::morphingItem() const {
	if (mFull) return mFull;
	if (mFullPull) return *mFullPull;
	return mFullSettling;
}

halnf TreeView::detailAmount() const {
	const halnf low = mStyle.compactBelowScale;
	const halnf high = mStyle.detailedAboveScale;
	if (high <= low) return scale() >= high ? 1.0f : 0.0f;
	const halnf t = std::clamp((scale() - low) / (high - low), 0.0f, 1.0f);
	return t * t * (3 - 2 * t);
}

Vec2F TreeView::pan() const { return mPan.value(); }

void TreeView::updateRects() {
	const std::vector<TreeStructure::Item>& items = mStructure.items();
	const halnf zoom = scale();
	const Vec2F size = mStyle.layout.card * zoom;
	const halnf preview = levelPreview();

	std::vector<Vec2F> world(items.size());
	for (size_t item = 0; item < items.size(); item++) {
		const auto position = mPositions.find(items[item].id);
		const Vec2F settled = position == mPositions.end() ? mCurrent.centers[item] : position->second.value();
		world[item] = preview > 0 ? mix(settled, mNext.centers[item], preview) : settled;
	}

	Vec2F offset = mPan.value();
	if (mPivotWorld) offset = mPivotScreen.value() - *mPivotWorld * zoom;
	else if (mPivot) {
		if (const std::optional<size_t> pivot = mStructure.indexOf(*mPivot))
			offset = mPivotScreen.value() - world[*pivot] * zoom;
	}
	mRects.resize(items.size());
	for (size_t item = 0; item < items.size(); item++) {
		Vec2F center = world[item] * zoom + offset;
		if (mTugged && items[item].id == *mTugged) center = center + mTug.value();
		mRects[item] = makeRect(center - size / 2.0f, size);
	}
	if (mPivot || mPivotWorld) mPan.jumpTo(offset);
}

bool TreeView::isInteractive(size_t item) const {
	if (item >= mRects.size() || mFull) return false;
	return isShownIndex(item) && displayedOpacity(item) > 0.5f;
}

void TreeView::arrangeCards(LayoutContext& context, const RectF& area) {
	if (area.size.x > 0 && area.size.y > 0 && (mFitPending || !(area.size == mLastSize))) {
		const bool resized = !mFitPending && !(area.size == mLastSize);
		mLastSize = area.size;
		if (mFitPending) fitInitially(area.size);
		else if (resized && mSelection) pivotOn(mSelection, area.size / 2.0f, false);
	}
	updateRects();
	const halnf detail = detailAmount();
	const RectF visible = makeRect({ 0, 0 }, area.size);
	const std::optional<TreeItemId> morphing = morphingItem();
	const std::optional<size_t> morphingIndex = morphing ? mStructure.indexOf(*morphing) : std::nullopt;
	if (morphingIndex) {
		if (children().back() != &mFrame) mFrame.bringToFront();
		context.place(mFrame, translated(mFrame.rectFor(mRects[*morphingIndex], visible), area.pos));
	}
	for (Widget* child : children()) {
		auto* card = dynamic_cast<TreeCard*>(child);
		if (!card) continue;
		const std::optional<size_t> index = mStructure.indexOf(card->id());
		if (!index) continue;
		const RectF rect = mRects[*index];
		const halnf opacity = displayedOpacity(*index);
		card->setVisible(opacity > 0.001f && overlaps(insetRect(visible, -kStackOffset * 2), rect));
		if (!card->isVisible()) continue;

		const TreeStructure::Item& item = mStructure.items()[*index];
		const bool folded = !item.children.empty() && !mExpansion.isOpen(item.id);
		const bool previewFolded = !item.children.empty() && (mCandidate ? !mCandidate->expansion.isOpen(item.id) : folded);
		const halnf preview = levelPreview();
		card->mStack = (folded ? 1.0f : 0.0f) + ((previewFolded ? 1.0f : 0.0f) - (folded ? 1.0f : 0.0f)) * preview;
		card->mHidden = item.descendants;
		card->mDetail = detail;
		card->mOpacity = opacity;
		card->mSelected = mSelected.contains(card->id());
		card->mCurrent = mSelection == card->id();
		const auto hover = mHover.find(card->id());
		card->mHover = hover == mHover.end() ? 0 : hover->second.value();
		card->content().setVisible(detail >= kContentShownDetail && isInteractive(*index));
		context.place(*card, translated(rect, area.pos));
	}
}

void TreeView::viewChanged() {
	startTicking();
	requestLayout();
	requestPaint();
}

halnf TreeView::clampScale(halnf value) const { return std::clamp(value, mStyle.minimumScale, mStyle.maximumScale); }

void TreeView::setView(Vec2F pan, halnf value, bool animated) {
	dropPivot();
	mFitPending = false;
	if (animated) {
		mPan.animateTo(pan);
		mScale.animateTo(clampScale(value));
	} else {
		mPan.jumpTo(pan);
		mScale.jumpTo(clampScale(value));
	}
	viewChanged();
}

void TreeView::pivotOn(std::optional<TreeItemId> id, std::optional<Vec2F> screen, bool animated) {
	if (!id || !mStructure.indexOf(*id)) return;
	updateRects();
	const RectF rect = mRects[*mStructure.indexOf(*id)];
	const Vec2F current = rect.pos + rect.size / 2.0f;
	if (mPivotWorld || !mPivot || *mPivot != *id) mPivotScreen.jumpTo(current);
	mPivotWorld.reset();
	mPivot = id;
	const Vec2F target = screen.value_or(current);
	if (animated) mPivotScreen.animateTo(target);
	else mPivotScreen.jumpTo(target);
	viewChanged();
}

void TreeView::pivotOnPoint(Vec2F local) {
	updateRects();
	const Vec2F offset = mPan.value();
	mPivot.reset();
	mPivotWorld = (local - offset) / scale();
	mPivotScreen.jumpTo(local);
	mPivotScreen.animateTo(local);
	viewChanged();
}

void TreeView::dropPivot() {
	mPivot.reset();
	mPivotWorld.reset();
}

void TreeView::followSubtree(TreeItemId id) {
	const std::optional<size_t> index = mStructure.indexOf(id);
	if (!index || mLastSize.x <= 0) return;
	const Vec2F anchor = mCurrent.centers[*index];
	Vec2F low = anchor;
	Vec2F high = anchor;
	for (size_t item = 0; item < mStructure.items().size(); item++) {
		if (!isShownIndex(item) || !mStructure.isAncestorOrSelf(*index, item)) continue;
		const Vec2F center = mCurrent.centers[item];
		low = { std::min(low.x, center.x), std::min(low.y, center.y) };
		high = { std::max(high.x, center.x), std::max(high.y, center.y) };
	}
	const Vec2F card = mStyle.layout.card;
	const halnf margin = mStyle.frameMargin;
	const Vec2F available = mLastSize - Vec2F(margin, margin) * 2.0f;
	const Vec2F extent = high - low + card;
	const halnf fit = std::min(available.x / std::max(extent.x, 1.0f), available.y / std::max(extent.y, 1.0f));
	const halnf zoom = clampScale(fit < mScale.target() ? snappedScale(fit) : mScale.target());

	pivotOn(id, std::nullopt, false);
	const Vec2F current = mPivotScreen.value();
	const Vec2F half = card * (zoom / 2.0f);
	const Vec2F before = (anchor - low) * zoom + half;
	const Vec2F after = (high - anchor) * zoom + half;
	const auto place = [&](halnf at, halnf lead, halnf trail, halnf size) {
		const halnf first = margin + lead;
		const halnf last = size - margin - trail;
		if (first > last) return (first + last) / 2;
		return std::clamp(at, first, last);
	};
	const Vec2F target = {
		place(current.x, before.x, after.x, mLastSize.x),
		place(current.y, before.y, after.y, mLastSize.y),
	};
	mPivotScreen.animateTo(target);
	mScale.animateTo(zoom);
	viewChanged();
}

Vec2F TreeView::fitExtent(const TreeExpansion& expansion) const {
	return TreePacking::pack(mStructure, expansion, mStyle.layout).extent;
}

void TreeView::fitInitially(Vec2F viewSize) {
	mFitPending = false;
	if (mStructure.empty()) return;
	const Vec2F available = viewSize - Vec2F(mStyle.frameMargin, mStyle.frameMargin) * 2.0f;
	const auto scaleFor = [&](Vec2F extent) {
		return std::min(1.0f, std::min(available.x / std::max(extent.x, 1.0f), available.y / std::max(extent.y, 1.0f)));
	};

	TreeExpansion expansion;
	size_t depth = mStructure.height();
	expansion.openTo(mStructure, std::nullopt, depth);
	while (depth > 0 && scaleFor(fitExtent(expansion)) < mStyle.compactBelowScale) {
		depth--;
		expansion.openTo(mStructure, std::nullopt, depth);
	}
	mExpansion = expansion;
	repack();
	syncItems(true);
	const Vec2F extent = mCurrent.extent;
	const halnf zoom = clampScale(snappedScale(scaleFor(extent)));
	mScale.jumpTo(zoom);
	mPan.jumpTo(viewSize / 2.0f - extent * (zoom / 2.0f));
	dropPivot();
}

void TreeView::frame(bool animated) {
	if (mStructure.empty() || mLastSize.x <= 0) {
		mFitPending = true;
		requestLayout();
		return;
	}
	if (mSelection) {
		pivotOn(mSelection, mLastSize / 2.0f, animated);
		return;
	}
	const Vec2F available = mLastSize - Vec2F(mStyle.frameMargin, mStyle.frameMargin) * 2.0f;
	const Vec2F extent = mCurrent.extent;
	halnf zoom = mScale.target();
	if (extent.x * zoom > available.x || extent.y * zoom > available.y)
		zoom = clampScale(std::min(available.x / std::max(extent.x, 1.0f), available.y / std::max(extent.y, 1.0f)));
	zoom = snappedScale(zoom);
	setView(mLastSize / 2.0f - extent * (zoom / 2.0f), zoom, animated);
}

void TreeView::centerOn(TreeItemId id, bool animated) {
	if (!mStructure.indexOf(id)) return;
	pivotOn(id, (mLastSize.x > 0 ? mLastSize : size()) / 2.0f, animated);
}

std::optional<size_t> TreeView::scopeIndex(std::optional<TreeItemId> scope) const {
	if (!scope) return std::nullopt;
	return mStructure.indexOf(*scope);
}

size_t TreeView::openDepth(std::optional<TreeItemId> scope) const {
	return mExpansion.openDepth(mStructure, scopeIndex(scope));
}

size_t TreeView::scopeHeight(std::optional<TreeItemId> scope) const {
	return mExpansion.subtreeHeight(mStructure, scopeIndex(scope));
}

void TreeView::setOpen(TreeItemId id, bool open) {
	const std::optional<size_t> index = mStructure.indexOf(id);
	if (!index || mStructure.items()[*index].children.empty() || mExpansion.isOpen(id) == open) return;
	bakePreview();
	mExpansion.setOpen(id, open);
	retarget(!ui());
	if (open && ui()) followSubtree(id);
}

void TreeView::openTo(std::optional<TreeItemId> scope, size_t depth) {
	bakePreview();
	mExpansion.openTo(mStructure, scopeIndex(scope), depth);
	retarget(!ui());
}

std::optional<TreeView::Configuration> TreeView::neighbour(int direction) const {
	const std::optional<size_t> root = scopeIndex(mSelection);
	Configuration next{ mExpansion };
	if (direction > 0) {
		const size_t open = mExpansion.openDepth(mStructure, root);
		if (open >= mExpansion.subtreeHeight(mStructure, root)) return std::nullopt;
		next.expansion.setLevel(mStructure, root, open, true);
		return next;
	}
	const std::optional<size_t> deepest = mExpansion.deepestOpenLevel(mStructure, root);
	if (!deepest) return std::nullopt;
	next.expansion.setLevel(mStructure, root, *deepest, false);
	return next;
}

halnf TreeView::levelPreview() const {
	if (!mCandidate) return 0;
	return std::max(0.0f, mLevelStep.offset() * halnf(mCandidateDirection));
}

void TreeView::bakePreview() { bakePreviewAt(levelPreview()); }

void TreeView::bakePreviewAt(halnf t) {
	if (!mCandidate) return;
	const std::vector<TreeStructure::Item>& items = mStructure.items();
	for (size_t item = 0; item < items.size(); item++) {
		const auto position = mPositions.find(items[item].id);
		if (position != mPositions.end()) position->second.jumpTo(mix(position->second.value(), mNext.centers[item], t));
		const auto fade = mOpacity.find(items[item].id);
		if (fade != mOpacity.end()) fade->second.jumpTo(displayedOpacity(item));
	}
	mCandidate.reset();
	mCandidateDirection = 0;
	mLevelStep.stop();
}

void TreeView::commit(const Configuration& configuration, halnf preview) {
	bakePreviewAt(preview);
	const bool opened = mCandidateDirectionCommitted > 0;
	mExpansion = configuration.expansion;
	retarget(false);
	if (opened && mSelection) followSubtree(*mSelection);
}

void TreeView::scaleAround(Vec2F local, halnf value) {
	const halnf after = clampScale(value);
	if (mSelection && mStructure.indexOf(*mSelection)) {
		if (!mPivot || *mPivot != *mSelection || mPivotWorld) pivotOn(mSelection, std::nullopt, false);
	} else if (!mPivotWorld || length(mPivotScreen.target() - local) > mStyle.dragThreshold) pivotOnPoint(local);
	mScale.animateTo(after);
	viewChanged();
}

std::optional<size_t> TreeView::nearestShown(Vec2F local) const {
	std::optional<size_t> nearest;
	halnf best = 0;
	for (size_t item = 0; item < mRects.size(); item++) {
		if (!isInteractive(item)) continue;
		const halnf distance = length(mRects[item].pos + mRects[item].size / 2.0f - local);
		if (nearest && distance >= best) continue;
		nearest = item;
		best = distance;
	}
	return nearest;
}

halnf TreeView::snappedScale(halnf value) const {
	const halnf low = mStyle.compactBelowScale;
	const halnf high = mStyle.detailedAboveScale;
	if (value <= low || value >= high) return value;
	return low;
}

void TreeView::settleCamera(Vec2F local, halnf value) {
	mCameraRest.reset();
	mCameraStep.stop();
	scaleAround(local, value);
}

void TreeView::zoomCamera(Vec2F local, halnf amount) {
	if (amount == 0) return;
	if (zoomFullView(local, amount)) return;
	const halnf low = mStyle.compactBelowScale;
	const halnf high = mStyle.detailedAboveScale;
	mCameraPointer = local;
	if (high <= low) {
		scaleAround(local, mScale.target() * std::exp(amount));
		return;
	}

	if (!mCameraRest) {
		const halnf current = mScale.target();
		if (current > low && current < high) {
			settleCamera(local, amount > 0 ? high : low);
			return;
		}
		const halnf wanted = current * std::exp(amount);
		if (current <= low && wanted > low) {
			mCameraRest = low;
			amount = std::log(wanted / low);
		} else if (current >= high && wanted < high) {
			mCameraRest = high;
			amount = std::log(wanted / high);
		} else {
			scaleAround(local, wanted);
			return;
		}
		mCameraStep.stop();
	}

	const halnf rest = *mCameraRest;
	const halnf inward = rest == low ? 1.0f : -1.0f;
	const halnf gap = std::log(high / low);
	const halnf inwardSteps = amount * inward / gap;
	if (mCameraStep.pulled() + inwardSteps <= 0) {
		const halnf outside = rest * std::exp((mCameraStep.pulled() + inwardSteps) * gap * inward);
		mCameraRest.reset();
		mCameraStep.stop();
		scaleAround(local, outside);
		return;
	}

	if (mCameraStep.push(inwardSteps) != 0) {
		settleCamera(local, rest == low ? high : low);
		return;
	}
	scaleAround(local, rest * std::exp(inward * gap * mCameraStep.pulledOffset()));
}

void TreeView::releaseCamera() {
	if (!mCameraRest) {
		mCameraStep.stop();
		return;
	}
	settleCamera(mCameraPointer, *mCameraRest);
}

halnf TreeView::fullViewScale() const {
	const Vec2F card = mStyle.layout.card;
	const Vec2F view = (mLastSize.x > 0 ? mLastSize : size()) * mStyle.fullViewFill;
	const halnf fill = std::min(view.x / std::max(card.x, 1.0f), view.y / std::max(card.y, 1.0f));
	return std::clamp(fill, mStyle.detailedAboveScale, mStyle.maximumScale);
}

std::optional<TreeItemId> TreeView::fullViewCandidate(Vec2F local) const {
	if (mSelection && mStructure.indexOf(*mSelection)) return mSelection;
	if (const std::optional<TreeItemId> under = itemAt(local)) return under;
	if (const std::optional<size_t> nearest = nearestShown(local)) return mStructure.items()[*nearest].id;
	return std::nullopt;
}

bool TreeView::zoomFullView(Vec2F local, halnf amount) {
	if (mFullPull) return pullIntoFullView(local, amount);
	if (amount <= 0 || mCameraRest) return false;

	const halnf threshold = fullViewScale();
	const halnf current = mScale.target();
	if (current < mStyle.detailedAboveScale - 0.0001f) return false;
	if (current * std::exp(amount) < threshold - 0.0001f) return false;
	if (current < threshold - 0.0001f) {
		scaleAround(local, threshold);
		return true;
	}
	return pullIntoFullView(local, amount);
}

bool TreeView::pullIntoFullView(Vec2F local, halnf amount) {
	if (!mFullPull) {
		const std::optional<TreeItemId> candidate = fullViewCandidate(local);
		if (!candidate) return false;
		mFullPull = candidate;
		mFullStep.stop();
		mFullSettling.reset();
	}

	const halnf steps = amount / mStyle.levelGap;
	if (mFullStep.pulled() + steps <= 0) {
		settleOutOfFullView(*mFullPull);
		return true;
	}
	if (mFullStep.push(steps) != 0) {
		enterFullView(*mFullPull);
		return true;
	}
	mFrame.showProgress(mFullStep.pulledOffset());
	viewChanged();
	return true;
}

void TreeView::releaseFullView() {
	if (mFullPull) settleOutOfFullView(*mFullPull);
	else mFullStep.stop();
}

void TreeView::settleOutOfFullView(TreeItemId item) {
	mFullPull.reset();
	mFullStep.stop();
	mFullSettling = item;
	mFrame.showProgress(0);
	viewChanged();
}

void TreeView::enterFullView(TreeItemId id, bool animated) {
	if (!mStructure.indexOf(id)) return;
	const bool changed = mFull != id;
	bakePreview();
	releaseCamera();
	mFullPull.reset();
	mFullStep.stop();
	mFullSettling.reset();
	mFull = id;
	if (mSelection != id) {
		select(id);
		if (mOnSelected) mOnSelected(id);
		selectionChanged();
	}
	if (mLastSize.x > 0) pivotOn(id, mLastSize / 2.0f, animated);
	mFrame.showProgress(1, animated);
	if (ui()) ui()->setKeyboardFocus(&mFrame);
	viewChanged();
	if (changed) fullViewChanged();
}

void TreeView::leaveFullView(bool animated) {
	mFullPull.reset();
	mFullStep.stop();
	if (!mFull) return;
	const TreeItemId was = *mFull;
	mFull.reset();
	if (ui() && ui()->keyboardFocus() && mFrame.isSelfOrAncestorOf(*ui()->keyboardFocus())) ui()->setKeyboardFocus(this);
	settleOutOfFullView(was);
	if (!animated) {
		mFrame.showProgress(0, false);
		mFullSettling.reset();
	}
	fullViewChanged();
}

void TreeView::fullViewChanged() {
	if (mOnFullViewChanged) mOnFullViewChanged(mFull);
}

TreeView::WheelAxis TreeView::wheelAxisFor(halnf topology, halnf camera) {
	if (!mWheelAxis)
		mWheelAxis = std::abs(topology) > std::abs(camera) ? WheelAxis::Topology : WheelAxis::Camera;
	mWheelIdleMs = 0;
	startTicking();
	return *mWheelAxis;
}

void TreeView::pull(Vec2F local, halnf amount) {
	if (mStructure.empty() || amount == 0 || isFullViewShown()) return;
	const halnf steps = amount / mStyle.levelGap;
	const int direction = (mLevelStep.pulled() + steps) > 0 ? 1 : -1;

	if (!mCandidate || mCandidateDirection != direction) {
		const std::optional<Configuration> next = neighbour(direction);
		if (mCandidate) bakePreview();
		if (!next) return;
		mCandidate = next;
		mCandidateDirection = direction;
		mLevelStep.stop();
		repack();
		if (mSelection) pivotOn(mSelection, std::nullopt, false);
		else if (const std::optional<size_t> anchor = nearestShown(local))
			pivotOn(mStructure.items()[*anchor].id, std::nullopt, false);
	}

	const halnf shown = levelPreview();
	if (mLevelStep.push(steps) != 0) {
		const Configuration next = *mCandidate;
		mCandidateDirectionCommitted = mCandidateDirection;
		mLevelStep.jumpTo(0);
		commit(next, shown);
		mCandidateDirectionCommitted = 0;
		return;
	}
	viewChanged();
}

void TreeView::releasePull() {
	if (!mCandidate) {
		mLevelStep.stop();
		return;
	}
	mLevelStep.release();
	viewChanged();
}

std::vector<TreeItemId> TreeView::selectedItems() const {
	std::vector<TreeItemId> ids;
	for (const TreeStructure::Item& item : mStructure.items())
		if (mSelected.contains(item.id)) ids.push_back(item.id);
	return ids;
}

void TreeView::setSelection(const std::vector<TreeItemId>& ids, std::optional<TreeItemId> current) {
	std::unordered_set<TreeItemId> selected;
	for (TreeItemId id : ids)
		if (mStructure.indexOf(id)) selected.insert(id);
	if (current && !selected.contains(*current)) current.reset();
	if (selected == mSelected && current == mSelection) return;
	const bool moved = current != mSelection;
	mSelected = std::move(selected);
	mSelection = current;
	if (moved && current) {
		reveal(*current);
		if (mLastSize.x > 0) pivotOn(current, mLastSize / 2.0f, true);
	}
	requestLayout();
	requestPaint();
}

void TreeView::reveal(TreeItemId id) {
	const std::optional<size_t> index = mStructure.indexOf(id);
	if (!index || isShownIndex(*index)) return;
	bakePreview();
	for (size_t parent = mStructure.items()[*index].parent; parent != TreeStructure::kNone;
			 parent = mStructure.items()[parent].parent)
		mExpansion.setOpen(mStructure.items()[parent].id, true);
	retarget(!ui());
}

void TreeView::select(std::optional<TreeItemId> id) {
	if (id) setSelection({ *id }, id);
	else setSelection({}, std::nullopt);
}

void TreeView::selectAll() {
	std::vector<TreeItemId> ids;
	for (const TreeStructure::Item& item : mStructure.items())
		ids.push_back(item.id);
	mSelected = std::unordered_set<TreeItemId>(ids.begin(), ids.end());
	if (!mSelection && !ids.empty()) mSelection = ids.front();
	requestLayout();
	requestPaint();
	selectionChanged();
}

void TreeView::requestDelete() {
	const std::vector<TreeItemId> ids = selectedItems();
	if (!ids.empty() && mOnDeleteRequested) mOnDeleteRequested(ids);
}

void TreeView::selectionChanged() {
	if (mOnSelectionChanged) mOnSelectionChanged();
}

void TreeView::selectByUser(TreeItemId id) {
	const bool changed = mSelected.size() != 1 || !mSelected.contains(id) || mSelection != id;
	select(id);
	if (mOnSelected) mOnSelected(id);
	if (changed) selectionChanged();
}

void TreeView::toggleByUser(TreeItemId id) {
	std::vector<TreeItemId> ids = selectedItems();
	std::optional<TreeItemId> current = mSelection;
	if (mSelected.contains(id)) {
		std::erase(ids, id);
		if (current == id) current = ids.empty() ? std::nullopt : std::optional(ids.back());
	} else {
		ids.push_back(id);
		current = id;
	}
	setSelection(ids, current);
	if (current && mOnSelected) mOnSelected(*current);
	selectionChanged();
}

std::optional<RectF> TreeView::selectionBox() const {
	if (mGesture != Gesture::Box || !mDragged) return std::nullopt;
	const Vec2F low = { std::min(mPressLocal.x, mBoxCorner.x), std::min(mPressLocal.y, mBoxCorner.y) };
	const Vec2F high = { std::max(mPressLocal.x, mBoxCorner.x), std::max(mPressLocal.y, mBoxCorner.y) };
	return makeRect(low, high - low);
}

void TreeView::updateBox(Vec2F local) {
	mBoxCorner = local;
	const std::optional<RectF> box = selectionBox();
	if (!box) return;
	std::unordered_set<TreeItemId> selected = mBoxBase;
	for (size_t item = 0; item < mRects.size(); item++)
		if (isInteractive(item) && overlaps(*box, mRects[item])) selected.insert(mStructure.items()[item].id);
	std::optional<TreeItemId> current = mSelection;
	if (current && !selected.contains(*current)) current.reset();
	if (!current && !selected.empty()) current = *selected.begin();
	mSelected = std::move(selected);
	mSelection = current;
	requestLayout();
	requestPaint();
}

std::optional<TreeItemState> TreeView::stateOf(TreeItemId id) const {
	const std::optional<size_t> index = mStructure.indexOf(id);
	if (!index) return std::nullopt;
	return isShownIndex(*index) ? TreeItemState::Shown : TreeItemState::Folded;
}

halnf TreeView::emergence(TreeItemId id) const {
	const std::optional<size_t> index = mStructure.indexOf(id);
	if (!index) return 0;
	return displayedOpacity(*index);
}

TreeCard* TreeView::card(TreeItemId id) const {
	const auto found = mCards.find(id);
	return found == mCards.end() ? nullptr : found->second.get();
}

std::optional<RectF> TreeView::cardRect(TreeItemId id) const {
	const std::optional<size_t> index = mStructure.indexOf(id);
	if (!index || *index >= mRects.size()) return std::nullopt;
	return mRects[*index];
}

std::optional<Vec2F> TreeView::portPosition(TreeItemId id, Port port) const {
	const std::optional<RectF> rect = cardRect(id);
	if (!rect) return std::nullopt;
	const halnf x = port == Port::Parent ? rect->pos.x : rect->pos.x + rect->size.x;
	return Vec2F(x, rect->pos.y + rect->size.y / 2);
}

std::optional<TreeItemId> TreeView::itemAt(Vec2F local) const {
	for (size_t item = mRects.size(); item-- > 0;)
		if (isInteractive(item) && contains(mRects[item], local)) return mStructure.items()[item].id;
	return std::nullopt;
}

std::optional<TreeView::PortHit> TreeView::portAt(Vec2F local) const {
	if (isFullViewShown()) return std::nullopt;
	if (portVisibility(mStyle.layout.card.y * scale()) < 0.5f) return std::nullopt;
	std::optional<PortHit> nearest;
	halnf best = mStyle.portHitRadius;
	for (size_t item = 0; item < mRects.size(); item++) {
		if (!isInteractive(item)) continue;
		const TreeItemId id = mStructure.items()[item].id;
		for (Port port : { Port::Parent, Port::Children }) {
			const halnf distance = length(*portPosition(id, port) - local);
			if (distance > best) continue;
			best = distance;
			nearest = PortHit{ id, port };
		}
	}
	return nearest;
}

size_t TreeView::hiddenDescendants(TreeItemId id) const {
	const std::optional<size_t> index = mStructure.indexOf(id);
	if (!index) return 0;
	const TreeStructure::Item& item = mStructure.items()[*index];
	if (item.children.empty() || mExpansion.isOpen(id)) return 0;
	return item.descendants;
}

bool TreeView::isAnimating() const {
	if (mScale.isAnimating() || mPan.isAnimating() || mPivotScreen.isAnimating()) return true;
	if (mLevelStep.isActive() || mTug.isAnimating() || mCameraStep.isPulling()) return true;
	if (mFrame.isAnimating() || mFullStep.isPulling()) return true;
	for (const auto& [id, position] : mPositions)
		if (position.isAnimating()) return true;
	for (const auto& [id, fade] : mOpacity)
		if (fade.isAnimating()) return true;
	for (const auto& [id, hover] : mHover)
		if (hover.isAnimating()) return true;
	return false;
}

bool TreeView::canLink(const PortHit& from, TreeItemId target) const {
	if (from.item == target) return false;
	if (from.port == Port::Children) return mModel.canReparent(target, from.item);
	return mModel.canReparent(from.item, target);
}

void TreeView::filterDescendantEvent(Event& event) {
	if (event.type() != Event::Type::Wheel && event.type() != Event::Type::Gesture) return;
	if (event.target() && mFrame.isSelfOrAncestorOf(*event.target())) return;
	onEvent(event);
}

bool TreeView::takesViewInput() const { return !mFull.has_value(); }

void TreeView::onEvent(Event& event) {
	const Vec2F local = event.windowPosition() - windowPosition();
	switch (event.type()) {
		case Event::Type::PointerDown: pressAt(event, local); break;
		case Event::Type::PointerMove: moveTo(event, local); break;
		case Event::Type::PointerUp: releaseAt(event, local); break;
		case Event::Type::Wheel:
			{
				if (!takesViewInput()) break;
				const halnf step = std::log(mStyle.wheelStep);
				halnf topology = event.horizontalWheelAmount();
				halnf camera = event.wheelAmount();
				if (event.modifiers().shift && topology == 0) std::swap(topology, camera);
				if (wheelAxisFor(topology, camera) == WheelAxis::Topology) pull(local, step * topology);
				else zoomCamera(local, step * camera);
				event.accept();
				break;
			}
		case Event::Type::Gesture:
			{
				if (!takesViewInput()) break;
				const TouchGesture& gesture = event.touchGesture();
				if (gesture.phase == GesturePhase::End) {
					releasePull();
					releaseCamera();
					releaseFullView();
					event.accept();
					break;
				}
				if (gesture.phase != GesturePhase::Update) break;
				if ((gesture.pan.x != 0 || gesture.pan.y != 0) && !isFullViewShown()) {
					if (mPivot || mPivotWorld) {
						mPivotScreen.jumpTo(mPivotScreen.value() + gesture.pan);
						if (mPivotScreen.isAnimating()) mPivotScreen.animateTo(mPivotScreen.target() + gesture.pan);
					} else mPan.jumpTo(mPan.value() + gesture.pan);
				}
				if (gesture.scale > 0 && gesture.scale != 1) {
					zoomCamera(gesture.center - windowPosition(), std::log(gesture.scale));
					mCameraStep.hold();
					mFullStep.hold();
				}
				viewChanged();
				event.accept();
				break;
			}
		case Event::Type::KeyDown:
			{
				const InputID key = event.keyCode();
				if (mFull) break;
				if (key == InputID::DELETE_KEY || key == InputID::BACKSPACE) {
					if (mSelected.empty() || !mOnDeleteRequested) break;
					requestDelete();
					event.accept();
					break;
				}
				if (key == InputID::ESCAPE) {
					if (mSelected.empty()) break;
					select(std::nullopt);
					selectionChanged();
					event.accept();
					break;
				}
				if (key == InputID::A && event.modifiers().control) {
					selectAll();
					event.accept();
					break;
				}
				if (key == InputID::ENTER || key == InputID::SPACE) {
					if (!mSelection) break;
					setOpen(*mSelection, !mExpansion.isOpen(*mSelection));
					event.accept();
					break;
				}
				if (key != InputID::LEFT && key != InputID::RIGHT && key != InputID::UP && key != InputID::DOWN) break;
				moveSelection(key);
				event.accept();
				break;
			}
		default: break;
	}
}

void TreeView::pressAt(Event& event, Vec2F local) {
	if (isFullViewShown()) {
		event.accept();
		return;
	}
	if (ui()) ui()->setKeyboardFocus(this);
	mPressLocal = local;
	mDragged = false;
	mPressedSelected = false;
	event.accept();

	if (event.button() == PointerButton::Middle || (event.button() == PointerButton::Primary && event.modifiers().alt)) {
		mGesture = Gesture::Pan;
		ui()->capturePointer(*this);
		return;
	}
	if (event.button() != PointerButton::Primary) return;
	const bool toggling = event.modifiers().control || event.modifiers().shift;

	if (const std::optional<PortHit> port = portAt(local)) {
		mLink = LinkDrag{ *port, local, std::nullopt, false };
		mGesture = Gesture::Link;
		ui()->capturePointer(*this);
		return;
	}

	if (const std::optional<TreeItemId> item = itemAt(local)) {
		if (toggling) {
			toggleByUser(*item);
			return;
		}
		mPressedSelected = mSelection == item && mSelected.size() == 1;
		if (!mPressedSelected) selectByUser(*item);
		mTugged = item;
		mTug.jumpTo({ 0, 0 });
		mGesture = Gesture::Tug;
		ui()->capturePointer(*this);
		return;
	}

	mBoxBase = toggling ? mSelected : std::unordered_set<TreeItemId>{};
	mBoxCorner = local;
	mGesture = Gesture::Box;
	ui()->capturePointer(*this);
}

void TreeView::moveTo(Event& event, Vec2F local) {
	if (mGesture == Gesture::None) {
		setHovered(itemAt(local));
		return;
	}
	event.accept();
	if (length(local - mPressLocal) >= mStyle.dragThreshold) mDragged = true;

	switch (mGesture) {
		case Gesture::Pan:
			if (!mDragged) break;
			if (mPivot || mPivotWorld) {
				const Vec2F target = mPivotScreen.target() + event.delta();
				mPivotScreen.jumpTo(mPivotScreen.value() + event.delta());
				mPivotScreen.animateTo(target);
			} else mPan.jumpTo(mPan.value() + event.delta());
			viewChanged();
			break;
		case Gesture::Tug:
			{
				const Vec2F pulled = local - mPressLocal;
				const halnf distance = length(pulled);
				const halnf resisted = MagneticStep::resisted(distance, mStyle.tugReach);
				mTug.jumpTo(distance > 0 ? pulled * (resisted / distance) : Vec2F(0.0f, 0.0f));
				requestLayout();
				requestPaint();
				break;
			}
		case Gesture::Link: updateLink(local); break;
		case Gesture::Box:
			if (mDragged) updateBox(local);
			break;
		case Gesture::None: break;
	}
}

void TreeView::releaseAt(Event& event, Vec2F local) {
	if (mGesture == Gesture::None) return;
	event.accept();
	const Gesture gesture = mGesture;
	mGesture = Gesture::None;
	if (ui()) ui()->releasePointer(*this);

	if (gesture == Gesture::Link) {
		updateLink(local);
		finishLink();
	}
	if (gesture == Gesture::Tug) {
		mTug.animateTo({ 0, 0 });
		startTicking();
		if (mPressedSelected && !mDragged && mSelection) setOpen(*mSelection, !mExpansion.isOpen(*mSelection));
	}
	if (gesture == Gesture::Box) {
		if (mDragged) selectionChanged();
		else if (mBoxBase.empty() && !mSelected.empty()) {
			select(std::nullopt);
			selectionChanged();
		}
		mBoxBase.clear();
	}
	mPressedSelected = false;
	requestLayout();
	requestPaint();
}

void TreeView::updateLink(Vec2F local) {
	if (!mLink) return;
	mLink->pointer = local;
	mLink->target = itemAt(local);
	mLink->valid = mLink->target && canLink(mLink->from, *mLink->target);
	requestPaint();
}

void TreeView::finishLink() {
	if (!mLink) return;
	const LinkDrag link = *mLink;
	mLink.reset();
	requestPaint();
	if (!mDragged) return;

	if (link.target) {
		if (!link.valid || !mOnReparent) return;
		const TreeItemId child = link.from.port == Port::Children ? *link.target : link.from.item;
		const TreeItemId parent = link.from.port == Port::Children ? link.from.item : *link.target;
		mExpansion.setOpen(parent, true);
		mOnReparent(child, parent);
		if (mStructure.indexOf(child)) reveal(child);
		return;
	}
	if (link.from.port != Port::Children || !mOnCreateChild) return;
	setOpen(link.from.item, true);
	mOnCreateChild(link.from.item);
}

void TreeView::setHovered(std::optional<TreeItemId> id) {
	if (id == mHovered) return;
	if (mHovered) mHover.try_emplace(*mHovered, 0.0f, Motion::quick()).first->second.animateTo(0);
	mHovered = id;
	if (id) mHover.try_emplace(*id, 0.0f, Motion::quick()).first->second.animateTo(1);
	startTicking();
}

void TreeView::onHoverChanged() {
	if (!isHovered()) setHovered(std::nullopt);
}

void TreeView::moveSelection(InputID key) {
	if (!mSelection) return;
	const std::optional<size_t> index = mStructure.indexOf(*mSelection);
	if (!index) return;
	const TreeStructure::Item& item = mStructure.items()[*index];
	const std::vector<size_t>& siblings =
		item.parent == TreeStructure::kNone ? mStructure.roots() : mStructure.items()[item.parent].children;
	const size_t position = size_t(std::find(siblings.begin(), siblings.end(), *index) - siblings.begin());

	std::optional<size_t> next;
	if (key == InputID::LEFT && item.parent != TreeStructure::kNone) next = item.parent;
	if (key == InputID::RIGHT && !item.children.empty()) next = item.children[(item.children.size() - 1) / 2];
	if (key == InputID::UP && position > 0) next = siblings[position - 1];
	if (key == InputID::DOWN && position + 1 < siblings.size()) next = siblings[position + 1];
	if (!next) return;
	if (key == InputID::RIGHT && !mExpansion.isOpen(item.id)) setOpen(item.id, true);
	selectByUser(mStructure.items()[*next].id);
}

void TreeView::advanceSteps(halnf deltaMs) {
	if (mCameraStep.advance(deltaMs).released) releaseCamera();
	if (mFullStep.advance(deltaMs).released) releaseFullView();
}

void TreeView::onTick(halnf deltaMs) {
	if (mWheelAxis) {
		mWheelIdleMs += deltaMs;
		if (mWheelIdleMs >= kWheelAxisReleaseMs) mWheelAxis.reset();
	}
	advanceSteps(deltaMs);
	bool active = mScale.advance(deltaMs);
	active |= mPan.advance(deltaMs);
	active |= mPivotScreen.advance(deltaMs);
	active |= mTug.advance(deltaMs);
	if (mTugged && !mTug.isAnimating() && mGesture != Gesture::Tug) mTugged.reset();
	active |= mLevelStep.advance(deltaMs).moving;
	active |= mFrame.isAnimating();
	if (mFullSettling && !mFrame.isAnimating()) mFullSettling.reset();
	if (mCandidate && !mLevelStep.isActive() && levelPreview() <= 0) {
		mCandidate.reset();
		mCandidateDirection = 0;
		repack();
	}
	for (auto& [id, position] : mPositions)
		active |= position.advance(deltaMs);
	for (auto& [id, fade] : mOpacity)
		active |= fade.advance(deltaMs);
	for (auto& [id, amount] : mHover)
		active |= amount.advance(deltaMs);
	std::erase_if(mHover, [this](const auto& entry) {
		return entry.first != mHovered && !entry.second.isAnimating() && entry.second.value() <= 0;
	});

	requestLayout();
	requestPaint();
	const bool pulling = mLevelStep.isPulling() || mCameraStep.isPulling() || mFullStep.isPulling();
	if (!active && !pulling && !mCandidate && !mWheelAxis) stopTicking();
}

void TreeView::paint(Painter& painter) {
	ScopedClip clip(painter, localRect());
	painter.fillRect(localRect(), theme().palette.window);
	paintLinks(painter);
	paintStacks(painter);
}

void TreeView::paintStacks(Painter& painter) const {
	for (const auto& [id, card] : mCards) {
		if (!card->isVisible() || card->mStack <= 0.001f || card->mHidden == 0) continue;
		const RectF rect = card->rect();
		const halnf offset = kStackOffset * card->mStack * decorationScale(rect.size.y);
		const RGBA color = Palette::mix(card->accent(), theme().palette.window, 0.45f);
		painter.fillRect(
			translated(rect, { offset, offset }),
			withAlpha(color, card->mStack * card->mOpacity),
			std::min(kCardRadius, rect.size.y / 2)
		);
	}
}

void TreeView::paintCurve(Painter& painter, Vec2F from, Vec2F to, const RGBA& color, halnf thickness) const {
	const halnf bend = std::max(std::abs(to.x - from.x) * 0.5f, 16.0f);
	const Vec2F controlFrom = from + Vec2F(bend, 0.0f);
	const Vec2F controlTo = to - Vec2F(bend, 0.0f);
	std::vector<Vec2F> points;
	points.reserve(kCurveSegments + 1);
	for (int step = 0; step <= kCurveSegments; step++) {
		const halnf t = halnf(step) / kCurveSegments;
		const halnf u = 1 - t;
		points.push_back(
			from * (u * u * u) + controlFrom * (3 * u * u * t) + controlTo * (3 * u * t * t) + to * (t * t * t)
		);
	}
	painter.polyline(points, color, thickness);
}

void TreeView::paintLinks(Painter& painter) const {
	const Theme& style = theme();
	const RectF visible = localRect();
	for (size_t item = 0; item < mRects.size(); item++) {
		const TreeStructure::Item& entry = mStructure.items()[item];
		const halnf opacity = displayedOpacity(item);
		if (entry.parent == TreeStructure::kNone || opacity <= 0.001f) continue;
		const RectF parent = mRects[entry.parent];
		const RectF child = mRects[item];
		const Vec2F from = { parent.pos.x + parent.size.x, parent.pos.y + parent.size.y / 2 };
		const Vec2F to = { child.pos.x, child.pos.y + child.size.y / 2 };
		if (!overlaps(visible, boundsOf({ from, to }))) continue;
		const bool related = mSelected.contains(entry.id) || mSelected.contains(mStructure.items()[entry.parent].id);
		const halnf alpha = opacity * (related || mSelected.empty() ? 0.9f : 0.45f);
		const RGBA color = related ? style.palette.text : style.palette.mutedText;
		const halnf thickness = mStyle.linkThickness * std::max(0.6f, decorationScale(child.size.y));
		paintCurve(painter, from, to, withAlpha(color, alpha), thickness * (related ? 1.6f : 1.0f));
	}

	for (const auto& [fromId, toId] : mModel.crossLinks()) {
		const std::optional<size_t> from = mStructure.indexOf(fromId);
		const std::optional<size_t> to = mStructure.indexOf(toId);
		if (!from || !to || !isInteractive(*from) || !isInteractive(*to)) continue;
		const RectF first = mRects[*from];
		const RectF second = mRects[*to];
		const bool related = mSelected.contains(fromId) || mSelected.contains(toId);
		paintCurve(
			painter,
			{ first.pos.x + first.size.x, first.pos.y + first.size.y * 0.75f },
			{ second.pos.x, second.pos.y + second.size.y * 0.75f },
			withAlpha(style.palette.warningText, related || mSelected.empty() ? 0.8f : 0.35f),
			mStyle.linkThickness
		);
	}
}

void TreeView::paintOverlay(Painter& painter) {
	ScopedClip clip(painter, localRect());
	const Theme& style = theme();
	const halnf cardHeight = mStyle.layout.card.y * scale();
	const halnf ports = morphingItem() ? 0.0f : portVisibility(cardHeight);
	const halnf portRadius = mStyle.portRadius * decorationScale(cardHeight);
	const RectF visible = insetRect(localRect(), -portRadius);
	for (size_t item = 0; ports > 0 && item < mRects.size(); item++) {
		if (!isInteractive(item)) continue;
		const TreeItemId id = mStructure.items()[item].id;
		for (Port port : { Port::Parent, Port::Children }) {
			const Vec2F center = *portPosition(id, port);
			if (!contains(visible, center)) continue;
			painter.fillCircle(center, portRadius, withAlpha(style.palette.window, ports));
			painter.strokeCircle(center, portRadius, withAlpha(style.palette.mutedText, ports), 1.5f * ports);
		}
	}
	if (const std::optional<RectF> box = selectionBox()) {
		painter.fillRect(*box, withAlpha(style.palette.accent, 0.12f), 2);
		painter.strokeRect(*box, withAlpha(style.palette.accent, 0.8f), 2, 1);
	}
	if (!mLink) return;

	const Vec2F anchor = *portPosition(mLink->from.item, mLink->from.port);
	const RGBA color = !mLink->target ? style.palette.mutedText
										 : mLink->valid ? style.palette.accent
																		: style.palette.warningText;
	if (mLink->from.port == Port::Children)
		paintCurve(painter, anchor, mLink->pointer, color, mStyle.linkThickness * 1.5f);
	else paintCurve(painter, mLink->pointer, anchor, color, mStyle.linkThickness * 1.5f);
	if (mLink->target)
		if (const std::optional<RectF> target = cardRect(*mLink->target))
			painter.strokeRect(*target, color, kCardRadius, 2);
}

#include "KanbanBoard.hpp"

#include "BoxLayout.hpp"
#include "Event.hpp"
#include "Layout.hpp"
#include "Painter.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kCardGap = 6;
	constexpr halnf kCardPadding = 10;
	constexpr halnf kCardLineGap = 4;
	constexpr halnf kCardAccentWidth = 4;
	constexpr halnf kCardAccentInset = 5;
	constexpr halnf kCardBadgeRadius = 4;
	constexpr halnf kColumnGap = 10;
	constexpr halnf kColumnPadding = 8;
	constexpr halnf kBoardPadding = 10;
	constexpr halnf kHeaderAccentHeight = 3;
	constexpr halnf kLiftedShadowOffset = 4;
	constexpr halnf kAutoScrollEdge = 40;
	constexpr halnf kAutoScrollSpeed = 0.8f;

	RGBA withAlpha(RGBA color, halnf alpha) {
		color.a *= alpha;
		return color;
	}

	bool movedPast(Vec2F from, Vec2F to, halnf threshold) {
		return std::fabs(to.x - from.x) >= threshold || std::fabs(to.y - from.y) >= threshold;
	}

	halnf edgeSpeed(halnf pointer, halnf start, halnf end) {
		if (end - start < kAutoScrollEdge * 3) return 0;
		if (pointer < start + kAutoScrollEdge) return -(start + kAutoScrollEdge - pointer) / kAutoScrollEdge;
		if (pointer > end - kAutoScrollEdge) return (pointer - (end - kAutoScrollEdge)) / kAutoScrollEdge;
		return 0;
	}
}

KanbanCard::KanbanCard(KanbanItemId id) :
	mId(id) {
	setDebugName("kanban card #" + std::to_string(id));
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<ColumnLayout>(kCardLineGap)
		.setPadding({ kCardPadding + kCardAccentInset, kCardPadding, kCardPadding + kCardBadgeRadius * 2, kCardPadding });

	mTitle.setWrap(true);
	mTitle.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mDetail.setWrap(true);
	mDetail.setTone(Label::Tone::Muted);
	mDetail.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mDetail.setVisible(false);

	addChild(mTitle);
	addChild(mDetail);
}

void KanbanCard::setContent(Widget& content) {
	if (mContent) removeChild(*mContent);
	mContent = &content;
	mTitle.setVisible(false);
	mDetail.setVisible(false);
	layout()->setPadding({ kCardPadding + kCardAccentInset, kCardPadding, kCardPadding, kCardPadding });
	content.setSizing(AxisSizing::fill(), AxisSizing::fill());
	addChild(content);
}

void KanbanCard::setFixedHeight(std::optional<halnf> height) {
	setSizing(AxisSizing::fill(), height ? AxisSizing::fixed(*height) : AxisSizing::fit());
	setClipsChildren(height.has_value());
	if (mContent) return;
	mTitle.setMaximumLines(height ? std::optional<size_t>(1) : std::nullopt);
	mDetail.setMaximumLines(height ? std::optional<size_t>(detailLinesFor(*height)) : std::nullopt);
}

size_t KanbanCard::detailLinesFor(halnf height) const {
	const Metrics& metrics = theme().metrics;
	const halnf detailHeight = height - kCardPadding * 2 - metrics.lineHeight - kCardLineGap;
	return size_t(std::max(1.0f, std::floor(detailHeight / metrics.lineHeight)));
}

void KanbanCard::setDetail(std::string detail) {
	mDetail.setVisible(!mContent && !detail.empty());
	mDetail.setText(std::move(detail));
}

void KanbanCard::setAccent(std::optional<RGBA> accent) {
	mAccent = accent;
	requestPaint();
}

void KanbanCard::setBadge(std::optional<RGBA> color) {
	mBadge = color;
	requestPaint();
}

void KanbanCard::setSelected(bool selected) {
	if (mSelected == selected) return;
	mSelected = selected;
	requestPaint();
}

void KanbanCard::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF rect = localRect();

	painter.fillRect(
		rect, isHovered() ? style.palette.controlHovered : style.palette.raisedSurface, style.metrics.cornerRadius
	);
	if (mAccent) {
		const RectF strip =
			makeRect(kCardAccentInset, kCardAccentInset, kCardAccentWidth, rect.size.y - kCardAccentInset * 2);
		painter.fillRect(strip, *mAccent, kCardAccentWidth / 2);
	}
	if (mBadge) {
		painter.fillCircle({ rect.size.x - kCardPadding, kCardPadding + kCardBadgeRadius }, kCardBadgeRadius, *mBadge);
	}
}

void KanbanCard::paintOverlay(Painter& painter) {
	if (!mSelected) return;
	const Theme& style = theme();
	painter.strokeRect(localRect(), style.palette.accent, style.metrics.cornerRadius, 2);
}

class KanbanCardStack::CardLayout : public Layout {
public:
	explicit CardLayout(KanbanCardStack& stack) :
		mStack(stack) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		if (axis == Axis::Horizontal) return Extent::shrinkable(0);

		halnf total = 0;
		size_t slots = 0;
		for (const KanbanCard* card : mStack.mCards) {
			if (!isArranged(context, *card)) continue;
			total += context.measure(*card, axis, crossLength).preferred;
			slots++;
		}
		if (mStack.mPlaceholderIndex) {
			total += mStack.mPlaceholderHeight;
			slots++;
		}
		if (slots > 1) total += kCardGap * halnf(slots - 1);
		return Extent::rigid(total);
	}

	void arrangeChildren(LayoutContext& context, const RectF& area) override {
		halnf y = area.pos.y;
		size_t slot = 0;
		mStack.mPlaceholderRect = makeRect(0, 0, 0, 0);

		const auto placePlaceholderAt = [&](size_t index) {
			if (mStack.mPlaceholderIndex != index) return;
			mStack.mPlaceholderRect = makeRect(area.pos.x, y, area.size.x, mStack.mPlaceholderHeight);
			y += mStack.mPlaceholderHeight + kCardGap;
		};

		for (KanbanCard* card : mStack.mCards) {
			if (!isArranged(context, *card)) continue;
			placePlaceholderAt(slot);
			const halnf height = context.measure(*card, Axis::Vertical, area.size.x).preferred;
			context.place(*card, makeRect(area.pos.x, y, area.size.x, height));
			land(*card);
			y += height + kCardGap;
			slot++;
		}
		placePlaceholderAt(slot);
	}

private:
	[[nodiscard]] static bool isArranged(const LayoutContext& context, const KanbanCard& card) {
		const auto& children = context.children();
		return std::find(children.begin(), children.end(), &card) != children.end();
	}

	void land(KanbanCard& card) {
		const auto landing = mStack.mLandings.find(&card);
		if (landing == mStack.mLandings.end()) return;

		const RectF from = makeRect(landing->second.pos - mStack.windowPosition(), landing->second.size);
		mStack.mLandings.erase(landing);
		card.animateGeometryFrom(from);
	}

	KanbanCardStack& mStack;
};

KanbanCardStack::KanbanCardStack() {
	setDebugName("kanban card stack");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	setLayout(std::make_unique<CardLayout>(*this));
}

void KanbanCardStack::setCards(
	const std::vector<KanbanCard*>& cards, const std::unordered_map<KanbanCard*, RectF>& landings
) {
	for (KanbanCard* card : mCards) {
		if (std::find(cards.begin(), cards.end(), card) == cards.end() && card->parent() == this) removeChild(*card);
	}
	for (KanbanCard* card : cards) {
		if (card->parent() != this) addChild(*card);
		const auto landing = landings.find(card);
		if (landing != landings.end()) mLandings[card] = landing->second;
	}
	mCards = cards;
	requestLayout();
}

void KanbanCardStack::removeCard(KanbanCard& card) {
	mCards.erase(std::remove(mCards.begin(), mCards.end(), &card), mCards.end());
	mLandings.erase(&card);
	if (card.parent() == this) removeChild(card);
	requestLayout();
}

void KanbanCardStack::setPlaceholder(std::optional<size_t> index, halnf height) {
	if (mPlaceholderIndex == index && mPlaceholderHeight == height) return;
	mPlaceholderIndex = index;
	mPlaceholderHeight = height;
	requestLayout();
	requestPaint();
}

std::optional<size_t> KanbanCardStack::indexOf(const KanbanCard& card) const {
	const auto position = std::find(mCards.begin(), mCards.end(), &card);
	if (position == mCards.end()) return std::nullopt;
	return size_t(position - mCards.begin());
}

size_t KanbanCardStack::insertionIndexAt(halnf windowY) const {
	size_t index = 0;
	for (const KanbanCard* card : mCards) {
		if (card->parent() != this) continue;
		const RectF rect = card->windowRect();
		if (windowY < rect.pos.y + rect.size.y / 2) return index;
		index++;
	}
	return index;
}

void KanbanCardStack::paint(Painter& painter) {
	if (!mPlaceholderIndex) return;
	const Theme& style = theme();
	painter.fillRect(mPlaceholderRect, withAlpha(style.palette.accent, 0.12f), style.metrics.cornerRadius);
	painter.strokeRect(mPlaceholderRect, style.palette.focusRing, style.metrics.cornerRadius, 1.5f);
}

KanbanColumnView::KanbanColumnView(const KanbanColumn& column) :
	mColumn(column) {
	setDebugName("kanban column " + column.title);
	setVisible(column.visible);
	emplaceLayout<ColumnLayout>(6).setPadding(Insets::uniform(kColumnPadding));

	mTitle.setText(column.title);
	mTitle.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mCount.setTone(Label::Tone::Muted);
	mHeader.setSizing(AxisSizing::fill(), AxisSizing::fit());
	const halnf accentSpace = column.accent ? kHeaderAccentHeight + 4 : 2;
	mHeader.emplaceLayout<RowLayout>(6).setPadding({ 2, 0, 2, accentSpace });
	mHeader.addChild(mTitle);
	mHeader.addChild(mCount);
	mOptions.setTooltip("Columns");
	mOptions.setOnClicked([this](const Event&) {
		mOptionsMenu.openAt(mOptions, mOptions.windowRect(), Placement::Below);
	});
	mHeader.addChild(mOptions);

	mScroller.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mScroller.setWheelStep(48);
	mScroller.setContent(&mStack);

	addChild(mHeader);
	addChild(mScroller);
	refreshCount();
}

void KanbanColumnView::setDropState(DropState state) {
	if (mDropState == state) return;
	mDropState = state;
	requestPaint();
}

void KanbanColumnView::refreshCount() {
	const std::string count = std::to_string(mStack.cards().size());
	mCount.setText(mColumn.cardLimit ? count + " / " + std::to_string(*mColumn.cardLimit) : count);
	const bool overLimit = mColumn.cardLimit && mStack.cards().size() > *mColumn.cardLimit;
	mCount.setTone(overLimit ? Label::Tone::Warning : Label::Tone::Muted);
}

void KanbanColumnView::paint(Painter& painter) {
	const Theme& style = theme();
	const halnf radius = style.metrics.surfaceCornerRadius;
	const RectF rect = localRect();

	painter.fillRect(rect, style.palette.surface, radius);
	if (mColumn.accent) {
		const RectF header = mHeader.rect();
		const halnf lineTop = header.pos.y + header.size.y - kHeaderAccentHeight;
		painter.fillRect(
			makeRect(kColumnPadding, lineTop, rect.size.x - kColumnPadding * 2, kHeaderAccentHeight),
			*mColumn.accent,
			kHeaderAccentHeight / 2
		);
	}
	if (mDropState == DropState::Accepted) painter.strokeRect(rect, style.palette.accent, radius, 2);
	if (mDropState == DropState::Refused) painter.strokeRect(rect, style.palette.windowCloseHovered, radius, 2);
}

class KanbanBoard::StripLayout : public Layout {
public:
	explicit StripLayout(KanbanBoard& board) :
		mBoard(board) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf) const override {
		const size_t count = context.children().size() + (mBoard.columnGapSlot() ? 1 : 0);
		if (axis == Axis::Vertical) return Extent::shrinkable(0);
		return Extent::shrinkable(stripLength(count, mBoard.mMinimumColumnWidth), 0);
	}

	void arrangeChildren(LayoutContext& context, const RectF& area) override {
		const std::optional<size_t> gap = mBoard.columnGapSlot();
		const size_t count = context.children().size() + (gap ? 1 : 0);
		if (count == 0) return;

		const halnf gaps = kColumnGap * halnf(count - 1);
		const halnf share = (area.size.x - gaps) / halnf(count);
		const halnf width = std::clamp(share, mBoard.mMinimumColumnWidth, mBoard.mMaximumColumnWidth);
		halnf x = area.pos.x;
		for (size_t slot = 0; slot < count; slot++) {
			const bool isGap = gap == slot;
			const size_t child = gap && slot > *gap ? slot - 1 : slot;
			const RectF rect = makeRect(x, area.pos.y, width, area.size.y);
			x += width + kColumnGap;
			if (isGap) {
				mBoard.mColumnGapRect = rect;
				continue;
			}
			Widget& column = *context.children()[child];
			context.place(column, rect);
			land(column);
		}
	}

private:
	void land(Widget& column) {
		const std::optional<Landing> landing = mBoard.mColumnLanding;
		if (!landing || landing->widget != &column) return;
		mBoard.mColumnLanding.reset();
		column.animateGeometryFrom(translated(landing->from, mBoard.mColumnStrip.windowPosition() * -1.0f));
	}

	[[nodiscard]] static halnf stripLength(size_t count, halnf width) {
		if (count == 0) return 0;
		return width * halnf(count) + kColumnGap * halnf(count - 1);
	}

	KanbanBoard& mBoard;
};

class KanbanBoard::BoardLayout : public Layout {
public:
	explicit BoardLayout(KanbanBoard& board) :
		mBoard(board) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext&, Axis, halnf) const override { return {}; }

	void arrangeChildren(LayoutContext& context, const RectF& area) override {
		context.place(mBoard.mColumnScroller, area);
		placeFullView(context, area);
		if (const Lift* lifted = mBoard.currentLift())
			context.place(*lifted->widget, makeRect(lifted->position, lifted->size));
	}

private:
	void placeFullView(LayoutContext& context, const RectF& area) {
		const std::optional<KanbanItemId> morphing = mBoard.morphingItem();
		if (!morphing) return;
		if (const KanbanCard* shown = mBoard.card(*morphing); shown && shown->ui())
			mBoard.mFullOrigin = translated(shown->windowRect(), mBoard.windowPosition() * -1.0f);
		const RectF local = makeRect({ 0, 0 }, area.size);
		context.place(mBoard.mFrame, translated(mBoard.mFrame.rectFor(mBoard.mFullOrigin, local), area.pos));
	}

	KanbanBoard& mBoard;
};

KanbanBoard::KanbanBoard(const KanbanModel& model) :
	mModel(model) {
	setDebugName("kanban board");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	setClipsChildren(true);
	setLayout(std::make_unique<BoardLayout>(*this));

	mColumnStrip.setDebugName("kanban columns");
	mColumnStrip.setLayout(std::make_unique<StripLayout>(*this));
	mColumnStrip.layout()->setPadding(Insets::uniform(kBoardPadding));
	mColumnScroller.setWheelStep(kColumnWheelStep);
	mColumnScroller.setContent(&mColumnStrip);
	addChild(mColumnScroller);
	mFrame.setOnBack([this] { leaveFullView(); });
	addChild(mFrame);
}

KanbanBoard::~KanbanBoard() {
	mDrag.reset();
	mColumnDrag.reset();
	mPress.reset();
	for (auto& column : mColumnViews)
		column->stack().removeAllChildren();
	removeAllChildren();
}

void KanbanBoard::setColumns(std::vector<KanbanColumn> columns) {
	cancelDrag();
	mColumns = std::move(columns);
	rebuildColumns();
	modelChanged();
}

void KanbanBoard::setCardHeight(std::optional<halnf> height) {
	mCardHeight = height;
	for (auto& [id, card] : mCards)
		card->setFixedHeight(height);
}

void KanbanBoard::setColumnWidthRange(halnf minimum, halnf maximum) {
	mMinimumColumnWidth = minimum;
	mMaximumColumnWidth = std::max(minimum, maximum);
	mColumnStrip.requestLayout();
}

void KanbanBoard::rebuildColumns() {
	for (auto& column : mColumnViews)
		column->stack().removeAllChildren();
	mColumnStrip.removeAllChildren();
	mColumnViews.clear();

	for (const KanbanColumn& column : mColumns) {
		mColumnViews.push_back(std::make_unique<KanbanColumnView>(column));
		mColumnStrip.addChild(*mColumnViews.back());
	}
	refreshOptionsMenus();
}

void KanbanBoard::refreshOptionsMenus() {
	for (const auto& view : mColumnViews) {
		view->optionsMenu().clear();
		fillOptionsMenu(view->optionsMenu());
	}
}

void KanbanBoard::fillOptionsMenu(Menu& menu) {
	for (size_t column = 0; column < mColumns.size(); column++) {
		MenuEntry entry = MenuEntry::toggle(
			mColumns[column].title,
			[this, column] { return isColumnVisible(column); },
			[this, column] { setColumnVisible(column, !isColumnVisible(column)); }
		);
		entry.isEnabled = [this, column] { return !isColumnVisible(column) || visibleColumnCount() > 1; };
		menu.add(std::move(entry));
	}
}

size_t KanbanBoard::visibleColumnCount() const {
	return size_t(std::count_if(mColumns.begin(), mColumns.end(), [](const KanbanColumn& column) {
		return column.visible;
	}));
}

void KanbanBoard::setColumnVisible(size_t column, bool visible) {
	if (column >= mColumns.size() || mColumns[column].visible == visible) return;
	if (!visible && visibleColumnCount() == 1) return;

	cancelDrag();
	mColumns[column].visible = visible;
	mColumnViews[column]->setVisible(visible);
	mColumnStrip.requestLayout();
	if (mOnColumnVisibilityChanged) mOnColumnVisibilityChanged(column, visible);
}

void KanbanBoard::moveColumn(size_t from, size_t to) {
	if (from >= mColumns.size() || to >= mColumns.size() || from == to) return;
	cancelDrag();

	KanbanColumn column = std::move(mColumns[from]);
	mColumns.erase(mColumns.begin() + std::ptrdiff_t(from));
	mColumns.insert(mColumns.begin() + std::ptrdiff_t(to), std::move(column));

	std::unique_ptr<KanbanColumnView> view = std::move(mColumnViews[from]);
	mColumnViews.erase(mColumnViews.begin() + std::ptrdiff_t(from));
	mColumnStrip.insertChild(to, *view);
	mColumnViews.insert(mColumnViews.begin() + std::ptrdiff_t(to), std::move(view));

	refreshOptionsMenus();
	if (mOnColumnMoved) mOnColumnMoved(from, to);
}

void KanbanBoard::modelChanged() { synchronize({}); }

void KanbanBoard::refreshCards() {
	for (auto& [id, card] : mCards)
		updateCard(*card);
}

void KanbanBoard::synchronize(std::unordered_map<KanbanCard*, RectF> landings) {
	std::vector<std::vector<KanbanItemId>> columnItems;
	for (size_t column = 0; column < mColumns.size(); column++)
		columnItems.push_back(mModel.items(column));

	const std::optional<KanbanItemId> dragged = draggedItem();
	if (dragged) {
		const bool stillListed = std::any_of(columnItems.begin(), columnItems.end(), [&](const auto& items) {
			return std::find(items.begin(), items.end(), *dragged) != items.end();
		});
		if (!stillListed) cancelDrag();
	}

	removeVanishedCards(columnItems);
	setColumnCards(columnItems, std::move(landings));
}

void KanbanBoard::removeVanishedCards(const std::vector<std::vector<KanbanItemId>>& columnItems) {
	const auto listed = [&](KanbanItemId id) {
		return std::any_of(columnItems.begin(), columnItems.end(), [id](const auto& items) {
			return std::find(items.begin(), items.end(), id) != items.end();
		});
	};

	for (auto entry = mCards.begin(); entry != mCards.end();) {
		if (listed(entry->first)) {
			++entry;
			continue;
		}
		for (auto& column : mColumnViews)
			column->stack().removeCard(*entry->second);
		if (mSelection == entry->first) mSelection.reset();
		if (mFull == entry->first) leaveFullView(false);
		if (mFullSettling == entry->first) mFullSettling.reset();
		entry = mCards.erase(entry);
	}
}

void KanbanBoard::setColumnCards(
	const std::vector<std::vector<KanbanItemId>>& columnItems, std::unordered_map<KanbanCard*, RectF> landings
) {
	const std::optional<KanbanItemId> dragged = draggedItem();
	std::vector<std::vector<KanbanCard*>> columnCards(mColumnViews.size());

	for (size_t column = 0; column < mColumnViews.size(); column++) {
		for (KanbanItemId id : columnItems[column]) {
			if (id == dragged) continue;
			KanbanCard& card = cardFor(id);
			const bool changesColumn = card.parent() && card.parent() != &mColumnViews[column]->stack();
			if (changesColumn && card.ui()) landings[&card] = card.windowRect();
			columnCards[column].push_back(&card);
		}
	}

	for (size_t column = 0; column < mColumnViews.size(); column++) {
		mColumnViews[column]->stack().setCards(columnCards[column], landings);
		mColumnViews[column]->refreshCount();
	}
}

KanbanCard& KanbanBoard::cardFor(KanbanItemId id) {
	auto& slot = mCards[id];
	if (!slot) {
		slot = mCardFactory ? mCardFactory(id) : std::make_unique<KanbanCard>(id);
		slot->setFixedHeight(mCardHeight);
	}
	updateCard(*slot);
	return *slot;
}

void KanbanBoard::updateCard(KanbanCard& card) {
	card.setSelected(mSelection == card.id());
	if (mCardUpdater) mCardUpdater(card);
}

KanbanCard* KanbanBoard::card(KanbanItemId id) const {
	const auto entry = mCards.find(id);
	return entry == mCards.end() ? nullptr : entry->second.get();
}

std::optional<std::pair<size_t, size_t>> KanbanBoard::locate(KanbanItemId id) const {
	const KanbanCard* found = card(id);
	if (!found) return std::nullopt;

	for (size_t column = 0; column < mColumnViews.size(); column++) {
		if (const std::optional<size_t> index = mColumnViews[column]->stack().indexOf(*found))
			return std::make_pair(column, *index);
	}
	return std::nullopt;
}

std::optional<size_t> KanbanBoard::columnOf(KanbanItemId id) const {
	if (mDrag && mDrag->card->id() == id) return mDrag->fromColumn;
	const auto location = locate(id);
	if (!location) return std::nullopt;
	return location->first;
}

void KanbanBoard::select(std::optional<KanbanItemId> id) {
	if (id && !card(*id)) id.reset();
	if (mSelection == id) return;

	if (mSelection)
		if (KanbanCard* previous = card(*mSelection)) previous->setSelected(false);
	mSelection = id;
	if (mSelection) card(*mSelection)->setSelected(true);
}

std::optional<KanbanItemId> KanbanBoard::draggedItem() const {
	if (!mDrag) return std::nullopt;
	return mDrag->card->id();
}

std::optional<KanbanDropTarget> KanbanBoard::dropTarget() const {
	if (!mDrag) return std::nullopt;
	return mDrag->target;
}

void KanbanBoard::onEvent(Event& event) {
	if (event.type() == Event::Type::Wheel) {
		scrollColumnsSideways(event);
		return;
	}
	handlePointer(event);
}

void KanbanBoard::scrollColumnsSideways(Event& event) {
	const halnf amount = event.horizontalWheelAmount();
	if (amount == 0 || !mColumnScroller.isScrollable() || isFullViewShown()) return;
	mColumnScroller.scrollBy(-amount * kColumnWheelStep);
	event.accept();
}

void KanbanBoard::filterDescendantEvent(Event& event) {
	if (event.target() && mFrame.isSelfOrAncestorOf(*event.target())) return;
	if (event.type() == Event::Type::Click) {
		handleClick(event);
		return;
	}
	handlePointer(event);
}

KanbanCard* KanbanBoard::cardAround(const Widget* target) const {
	for (const Widget* current = target; current && current != this; current = current->parent()) {
		if (const auto* found = dynamic_cast<const KanbanCard*>(current)) return card(found->id());
	}
	return nullptr;
}

void KanbanBoard::handleClick(Event& event) {
	KanbanCard* clicked = cardAround(event.target());
	if (mSuppressNextClick) {
		mSuppressNextClick = false;
		event.accept();
		return;
	}
	if (!clicked || mDrag) return;

	const KanbanItemId id = clicked->id();
	if (ui()) ui()->setKeyboardFocus(this);
	select(id);
	if (mOnSelected) mOnSelected(id);
	if (event.clickCount() != 2) return;
	if (mOnActivated) mOnActivated(id);
	enterFullView(id);
}

std::optional<KanbanItemId> KanbanBoard::morphingItem() const {
	if (mFull) return mFull;
	if (mFullSettling && mFrame.isShown()) return mFullSettling;
	return std::nullopt;
}

void KanbanBoard::enterFullView(KanbanItemId id, bool animated) {
	KanbanCard* shown = card(id);
	if (!shown) return;
	cancelDrag();
	const bool changed = mFull != id;
	mFull = id;
	mFullSettling.reset();
	if (shown->ui()) mFullOrigin = translated(shown->windowRect(), windowPosition() * -1.0f);
	mFrame.showProgress(1, animated);
	if (ui()) ui()->setKeyboardFocus(&mFrame);
	startTicking();
	requestLayout();
	if (changed) fullViewChanged();
}

void KanbanBoard::leaveFullView(bool animated) {
	if (!mFull) return;
	mFullSettling = mFull;
	mFull.reset();
	if (ui() && ui()->keyboardFocus() && mFrame.isSelfOrAncestorOf(*ui()->keyboardFocus())) ui()->setKeyboardFocus(this);
	mFrame.showProgress(0, animated);
	if (!animated) mFullSettling.reset();
	mColumnScroller.setVisible(true);
	startTicking();
	requestLayout();
	fullViewChanged();
}

void KanbanBoard::fullViewChanged() {
	if (mOnFullViewChanged) mOnFullViewChanged(mFull);
}

void KanbanBoard::handleKey(Event& event) {
	const InputID key = event.keyCode();
	if (key == InputID::ESCAPE && currentLift()) {
		cancelDrag();
		event.accept();
		return;
	}

	if (currentLift() || isFullViewShown()) return;
	if (key == InputID::DELETE_KEY || key == InputID::BACKSPACE) {
		if (!mSelection || !mOnDeleteRequested) return;
		mOnDeleteRequested(*mSelection);
		event.accept();
		return;
	}

	const std::optional<KanbanItemId> next = neighbourOfSelection(key);
	if (!next) return;
	select(next);
	if (mOnSelected) mOnSelected(*next);
	event.accept();
}

std::optional<KanbanItemId> KanbanBoard::neighbourOfSelection(InputID key) const {
	const bool vertical = key == InputID::UP || key == InputID::DOWN;
	const bool horizontal = key == InputID::LEFT || key == InputID::RIGHT;
	if (!vertical && !horizontal) return std::nullopt;

	const std::optional<std::pair<size_t, size_t>> at = mSelection ? locate(*mSelection) : std::nullopt;
	if (!at) return firstCard();
	const auto [column, index] = *at;

	if (vertical) {
		const std::vector<KanbanCard*>& cards = mColumnViews[column]->stack().cards();
		if (key == InputID::UP) return index > 0 ? std::optional(cards[index - 1]->id()) : std::nullopt;
		return index + 1 < cards.size() ? std::optional(cards[index + 1]->id()) : std::nullopt;
	}

	const int step = key == InputID::LEFT ? -1 : 1;
	for (long other = long(column) + step; other >= 0 && other < long(mColumnViews.size()); other += step) {
		KanbanColumnView& view = *mColumnViews[size_t(other)];
		const std::vector<KanbanCard*>& cards = view.stack().cards();
		if (!view.isVisible() || cards.empty()) continue;
		return cards[std::min(index, cards.size() - 1)]->id();
	}
	return std::nullopt;
}

std::optional<KanbanItemId> KanbanBoard::firstCard() const {
	for (const auto& view : mColumnViews)
		if (view->isVisible() && !view->stack().cards().empty()) return view->stack().cards().front()->id();
	return std::nullopt;
}

void KanbanBoard::handlePointer(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			{
				if (event.button() != PointerButton::Primary || currentLift() || isFullViewShown()) return;
				mSuppressNextClick = false;
				const std::optional<size_t> column = columnHeaderAround(event.target());
				KanbanCard* pressed = column ? nullptr : cardAround(event.target());
				if (!column && !pressed) return;
				mPress = Press{ pressed, column, event.windowPosition(), false };
				return;
			}

		case Event::Type::LongPress:
			if (!mPress || currentLift() || !ui() || !ui()->isTouchMode()) return;
			mPress->held = true;
			ui()->capturePointer(*this);
			event.accept();
			return;

		case Event::Type::PointerMove:
			if (currentLift()) {
				updateDrag(event.windowPosition());
				event.accept();
				return;
			}
			if (!mPress || !movedPast(mPress->windowPoint, event.windowPosition(), kDragThreshold)) return;
			if (ui() && ui()->isTouchMode() && !mPress->held) {
				mPress.reset();
				return;
			}
			if (mPress->card) beginDrag();
			else beginColumnDrag();
			updateDrag(event.windowPosition());
			event.accept();
			return;

		case Event::Type::PointerUp:
			if (event.button() != PointerButton::Primary) return;
			if (mDrag || mColumnDrag) {
				if (mDrag) finishDrag(true);
				else finishColumnDrag(true);
				event.accept();
				return;
			}
			if (mPress && mPress->held && ui()) ui()->releasePointer(*this);
			mPress.reset();
			return;

		case Event::Type::KeyDown: handleKey(event); return;

		default: return;
	}
}

std::optional<size_t> KanbanBoard::columnHeaderAround(const Widget* target) const {
	if (!target) return std::nullopt;
	for (size_t column = 0; column < mColumnViews.size(); column++) {
		KanbanColumnView& view = *mColumnViews[column];
		if (!view.header().isSelfOrAncestorOf(*target)) continue;
		if (view.optionsButton().isSelfOrAncestorOf(*target)) return std::nullopt;
		return column;
	}
	return std::nullopt;
}

KanbanBoard::Lift KanbanBoard::lift(Widget& widget, Vec2F windowPoint) {
	Lift lifted;
	lifted.widget = &widget;
	lifted.size = widget.size();
	lifted.position = widget.windowPosition() - windowPosition();
	lifted.grabOffset = windowPoint - widget.windowPosition();
	lifted.pointer = windowPoint;
	mPress.reset();

	widget.setGeometryAnimated(false);
	addChild(widget);
	if (ui()) {
		lifted.focusBeforeLift = ui()->keyboardFocus();
		ui()->capturePointer(*this);
		ui()->setKeyboardFocus(this);
	}
	setCursor(CursorShape::Grabbing);
	startTicking();
	requestLayout();
	return lifted;
}

void KanbanBoard::drop(const Lift& lifted) {
	stopTicking();
	setCursor(CursorShape::Default);
	removeChild(*lifted.widget);
	lifted.widget->setGeometryAnimated(true);
	if (ui()) {
		ui()->releasePointer(*this);
		if (ui()->keyboardFocus() == this) ui()->setKeyboardFocus(lifted.focusBeforeLift);
	}
}

const KanbanBoard::Lift* KanbanBoard::currentLift() const {
	if (mDrag) return &mDrag->lift;
	if (mColumnDrag) return &mColumnDrag->lift;
	return nullptr;
}

void KanbanBoard::beginDrag() {
	KanbanCard& dragged = *mPress->card;
	const auto origin = locate(dragged.id());
	if (!origin) {
		mPress.reset();
		return;
	}

	Drag drag;
	drag.card = &dragged;
	drag.fromColumn = origin->first;
	drag.fromIndex = origin->second;
	drag.lift = lift(dragged, mPress->windowPoint);

	KanbanCardStack& stack = mColumnViews[drag.fromColumn]->stack();
	stack.removeCard(dragged);
	stack.setPlaceholder(drag.fromIndex, drag.lift.size.y);
	mDrag = drag;
}

void KanbanBoard::beginColumnDrag() {
	const size_t column = *mPress->column;
	ColumnDrag drag;
	drag.column = column;
	drag.destination = column;
	drag.lift = lift(*mColumnViews[column], mPress->windowPoint);
	mColumnDrag = drag;
	mColumnStrip.requestLayout();
}

void KanbanBoard::updateDrag(Vec2F windowPoint) {
	if (mDrag) {
		mDrag->lift.pointer = windowPoint;
		mDrag->lift.position = windowPoint - mDrag->lift.grabOffset - windowPosition();
		showDropTarget(dropTargetAt(windowPoint));
	}
	if (mColumnDrag) {
		mColumnDrag->lift.pointer = windowPoint;
		mColumnDrag->lift.position.x = windowPoint.x - mColumnDrag->lift.grabOffset.x - windowPosition().x;
		mColumnDrag->destination = columnDestinationAt(mColumnDrag->lift.position.x + windowPosition().x);
		mColumnStrip.requestLayout();
	}
	requestLayout();
	requestPaint();
}

void KanbanBoard::finishDrag(bool commit) {
	const Drag drag = *mDrag;
	const bool accepted = commit && drag.target && drag.target->accepted;
	const std::optional<KanbanMove> move =
		accepted ? std::optional<KanbanMove>(moveTo(drag.target->column, drag.target->index)) : std::nullopt;
	const RectF landing = drag.card->windowRect();

	showDropTarget(std::nullopt);
	mDrag.reset();
	mSuppressNextClick = commit;
	drop(drag.lift);

	if (move && !isNoOp(*move) && mOnMoved) mOnMoved(*move);
	synchronize({ { drag.card, landing } });
}

void KanbanBoard::finishColumnDrag(bool commit) {
	const ColumnDrag drag = *mColumnDrag;
	const RectF landing = drag.lift.widget->windowRect();

	mColumnDrag.reset();
	mSuppressNextClick = commit;
	drop(drag.lift);
	mColumnStrip.insertChild(drag.column, *drag.lift.widget);
	mColumnLanding = Landing{ drag.lift.widget, landing };

	if (commit) moveColumn(drag.column, drag.destination);
}

void KanbanBoard::cancelDrag() {
	mPress.reset();
	if (mDrag) finishDrag(false);
	if (mColumnDrag) finishColumnDrag(false);
}

std::optional<size_t> KanbanBoard::columnDropIndex() const {
	if (!mColumnDrag) return std::nullopt;
	return mColumnDrag->destination;
}

size_t KanbanBoard::columnDestinationAt(halnf liftedLeft) const {
	const halnf liftedRight = liftedLeft + mColumnDrag->lift.size.x;
	const halnf stripX = mColumnStrip.windowPosition().x;
	size_t destination = 0;
	size_t slot = 0;
	for (size_t column = 0; column < mColumnViews.size(); column++) {
		if (column == mColumnDrag->column) continue;
		const halnf center = stripX + mColumnViews[column]->targetRect().center().x;
		const halnf facingEdge = slot < mColumnDrag->destination ? liftedLeft : liftedRight;
		if (mColumns[column].visible && facingEdge > center) destination = slot + 1;
		slot++;
	}
	return destination;
}

std::optional<size_t> KanbanBoard::columnGapSlot() const {
	if (!mColumnDrag) return std::nullopt;
	size_t visibleBefore = 0;
	size_t slot = 0;
	for (size_t column = 0; column < mColumnViews.size() && slot < mColumnDrag->destination; column++) {
		if (column == mColumnDrag->column) continue;
		if (mColumns[column].visible) visibleBefore++;
		slot++;
	}
	return visibleBefore;
}

void KanbanBoard::onTick(halnf deltaMs) {
	const bool covered = mFull && !mFrame.isAnimating();
	if (mColumnScroller.isVisible() == covered) mColumnScroller.setVisible(!covered);
	if (mFullSettling && !mFrame.isShown()) mFullSettling.reset();
	if (isFullViewShown()) {
		requestLayout();
		requestPaint();
	}
	if (currentLift()) autoScroll(deltaMs);
	if (!currentLift() && !mFrame.isAnimating()) stopTicking();
}

void KanbanBoard::autoScroll(halnf deltaMs) {
	const Vec2F pointer = currentLift()->pointer;
	const RectF board = windowRect();
	bool scrolled = false;

	const halnf horizontal = edgeSpeed(pointer.x, board.pos.x, board.pos.x + board.size.x);
	if (horizontal != 0 && mColumnScroller.isScrollable()) {
		mColumnScroller.jumpTo(mColumnScroller.offset() + horizontal * kAutoScrollSpeed * deltaMs);
		scrolled = true;
	}

	const std::optional<size_t> column = mDrag ? columnAt(pointer.x) : std::nullopt;
	if (column) {
		ScrollView& scroller = mColumnViews[*column]->scroller();
		const RectF area = scroller.windowRect();
		const halnf vertical = edgeSpeed(pointer.y, area.pos.y, area.pos.y + area.size.y);
		if (vertical != 0 && scroller.isScrollable()) {
			scroller.jumpTo(scroller.offset() + vertical * kAutoScrollSpeed * deltaMs);
			scrolled = true;
		}
	}

	if (scrolled) updateDrag(pointer);
}

void KanbanBoard::showDropTarget(const std::optional<KanbanDropTarget>& target) {
	if (mDrag) mDrag->target = target;

	for (size_t column = 0; column < mColumnViews.size(); column++) {
		KanbanColumnView& view = *mColumnViews[column];
		const bool targeted = target && target->column == column;
		const bool manual = mColumns[column].ordering == KanbanOrdering::Manual;
		const bool showsSlot = targeted && target->accepted && manual;
		view.stack().setPlaceholder(
			showsSlot ? std::optional<size_t>(target->index) : std::nullopt, mDrag ? mDrag->lift.size.y : 0
		);

		KanbanColumnView::DropState state = KanbanColumnView::DropState::None;
		if (targeted && !target->accepted) state = KanbanColumnView::DropState::Refused;
		else if (targeted && (!manual || column != mDrag->fromColumn)) state = KanbanColumnView::DropState::Accepted;
		view.setDropState(state);
	}
}

std::optional<size_t> KanbanBoard::columnAt(halnf windowX) const {
	std::optional<size_t> nearest;
	halnf nearestDistance = kColumnGap;
	for (size_t column = 0; column < mColumnViews.size(); column++) {
		if (!mColumns[column].visible) continue;
		const RectF rect = mColumnViews[column]->windowRect();
		if (windowX >= rect.pos.x && windowX < rect.pos.x + rect.size.x) return column;
		const halnf distance = std::min(std::fabs(windowX - rect.pos.x), std::fabs(windowX - rect.pos.x - rect.size.x));
		if (distance <= nearestDistance) {
			nearest = column;
			nearestDistance = distance;
		}
	}
	return nearest;
}

std::optional<KanbanDropTarget> KanbanBoard::dropTargetAt(Vec2F windowPoint) const {
	if (!contains(windowRect(), windowPoint)) return std::nullopt;
	const std::optional<size_t> column = columnAt(windowPoint.x);
	if (!column) return std::nullopt;

	const KanbanCardStack& stack = mColumnViews[*column]->stack();
	const bool manual = mColumns[*column].ordering == KanbanOrdering::Manual;
	const size_t index = manual ? stack.insertionIndexAt(windowPoint.y) : stack.cards().size();
	return KanbanDropTarget{ *column, index, accepts(*column, index) };
}

bool KanbanBoard::accepts(size_t column, size_t index) const {
	const KanbanMove move = moveTo(column, index);
	if (isNoOp(move)) return true;

	if (column != mDrag->fromColumn) {
		const KanbanColumn& destination = mColumns[column];
		if (!destination.acceptsDrops) return false;
		const size_t occupied = mColumnViews[column]->stack().cards().size();
		if (destination.cardLimit && occupied >= *destination.cardLimit) return false;
	}
	return mModel.canMove(move);
}

KanbanMove KanbanBoard::moveTo(size_t column, size_t index) const {
	return { mDrag->card->id(), mDrag->fromColumn, mDrag->fromIndex, column, index };
}

bool KanbanBoard::isNoOp(const KanbanMove& move) const {
	if (move.fromColumn != move.toColumn) return false;
	return move.fromIndex == move.toIndex || mColumns[move.toColumn].ordering == KanbanOrdering::ModelDefined;
}

const Widget* KanbanBoard::raisedChild() const {
	if (const Lift* lifted = currentLift()) return lifted->widget;
	return isFullViewShown() ? &mFrame : nullptr;
}

void KanbanBoard::paintRaisedBackdrop(Painter& painter, const RectF& childRect) {
	const Theme& style = theme();
	if (!currentLift()) {
		RGBA cover = style.palette.window;
		cover.a *= std::clamp(mFrame.progress(), 0.0f, 1.0f);
		painter.fillRect(localRect(), cover);
		return;
	}
	if (mColumnDrag) {
		const RectF slot = translated(mColumnGapRect, mColumnStrip.windowPosition() - windowPosition());
		painter.strokeRect(slot, style.palette.focusRing, style.metrics.surfaceCornerRadius);
	}
	painter.fillRect(
		makeRect(childRect.pos.x + 2, childRect.pos.y + kLiftedShadowOffset, childRect.size.x, childRect.size.y),
		style.palette.shadow,
		style.metrics.cornerRadius
	);
}

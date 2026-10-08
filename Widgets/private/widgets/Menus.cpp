#include "Menus.hpp"

#include <algorithm>

#include "BoxLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "WindowHost.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kSubmenuArrowWidth = 16;
	constexpr halnf kWindowDragThreshold = 4;

	PopupOptions menuPopupOptions(Placement placement, halnf gap) {
		PopupOptions options;
		options.placement = placement;
		options.align = PlacementAlign::Start;
		options.gap = gap;
		return options;
	}
}

PopupButton::PopupButton(std::string text) :
	Button(std::move(text)) {
	setDebugName("popup button");
}

void PopupButton::setPopupContent(Widget* content) {
	if (mPopupContent && isPopupOpen()) closePopup();
	mPopupContent = content;
}

void PopupButton::openPopup() {
	if (!mPopupContent || !ui()) return;
	if (mOnOpening) mOnOpening();

	PopupRequest request;
	request.content = mPopupContent;
	request.owner = this;
	request.anchor = windowRect();
	request.options = mOptions;

	ui()->openPopup(request);
	refreshVisualState();
}

void PopupButton::openPopupFromHover() {
	openPopup();
	mOpenedByHover = isPopupOpen();
	if (mOpenedByHover && ui()) mHoverOpenPoint = ui()->pointerPosition();
}

void PopupButton::closePopup() {
	mOpenedByHover = false;
	mKeepOpenOnActivation = false;
	if (mPopupContent && ui()) ui()->closePopup(*mPopupContent);
	refreshVisualState();
}

bool PopupButton::isPopupOpen() const { return mPopupContent && ui() && ui()->isPopupOpen(*mPopupContent); }

void PopupButton::onEvent(Event& event) {
	if (event.type() == Event::Type::PointerDown && event.button() == PointerButton::Primary) {
		mKeepOpenOnActivation = mOpenedByHover && isPopupOpen() && event.windowPosition() == mHoverOpenPoint;
		mOpenedByHover = false;
	}
	Button::onEvent(event);
}

void PopupButton::onActivated() {
	if (mKeepOpenOnActivation) {
		mKeepOpenOnActivation = false;
		if (isPopupOpen()) return;
	}
	if (isPopupOpen()) closePopup();
	else openPopup();
}

void PopupButton::onHoverChanged() {
	Button::onHoverChanged();
	if (!isHovered()) {
		mOpenedByHover = false;
		mKeepOpenOnActivation = false;
		return;
	}

	if (mOpenOnHover && !isPopupOpen()) openPopupFromHover();
	if (mOnHovered) mOnHovered();
}

RGBA PopupButton::backgroundTarget() const {
	if (isPopupOpen()) return theme().palette.controlPressed;
	return Button::backgroundTarget();
}

MenuEntry MenuEntry::item(std::string text, std::function<void()> action, std::string shortcut) {
	MenuEntry entry;
	entry.kind = Kind::Action;
	entry.text = std::move(text);
	entry.action = std::move(action);
	entry.shortcut = std::move(shortcut);
	return entry;
}

MenuEntry MenuEntry::toggle(std::string text, std::function<bool()> isChecked, std::function<void()> action) {
	MenuEntry entry;
	entry.kind = Kind::Toggle;
	entry.text = std::move(text);
	entry.isChecked = std::move(isChecked);
	entry.action = std::move(action);
	return entry;
}

MenuEntry MenuEntry::separator() {
	MenuEntry entry;
	entry.kind = Kind::Separator;
	return entry;
}

MenuEntry MenuEntry::nested(std::string text, Menu& menu) {
	MenuEntry entry;
	entry.kind = Kind::Submenu;
	entry.text = std::move(text);
	entry.submenu = &menu;
	return entry;
}

MenuItem::MenuItem(Menu& menu, MenuEntry entry) :
	mMenu(menu),
	mEntry(std::move(entry)) {
	setDebugName("menu item: " + mEntry.text);
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	refresh();
}

void MenuItem::refresh() {
	if (mEntry.isEnabled) setEnabled(mEntry.isEnabled());

	std::string text = mEntry.dynamicText ? mEntry.dynamicText() : mEntry.text;
	if (text != mText) {
		mText = std::move(text);
		requestLayout();
	}
	requestPaint();
}

Extent MenuItem::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;

	if (axis == Axis::Vertical) return Extent::rigid(metrics.menuItemHeight);

	halnf width = metrics.controlPadding.total(Axis::Horizontal) + metrics.indicatorSize + metrics.spacing;
	width += textWidth(mText, metrics.fontSize);
	if (!mEntry.shortcut.empty()) width += metrics.spacing * 4 + textWidth(mEntry.shortcut, metrics.smallFontSize);
	if (mEntry.kind == MenuEntry::Kind::Submenu) width += kSubmenuArrowWidth;
	return Extent::natural(width);
}

void MenuItem::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF content = style.metrics.controlPadding.shrink(localRect());
	RGBA textColor = isEffectivelyEnabled() ? style.palette.text : style.palette.disabledText;
	if (mEntry.muted && isEffectivelyEnabled()) textColor = style.palette.mutedText;

	painter.fillRect(localRect(), background(), style.metrics.cornerRadius);

	const bool checked = mEntry.isChecked && mEntry.isChecked();
	if (mEntry.icon.isValid() && !checked) {
		const halnf side = style.metrics.indicatorSize * 1.25f;
		const RectF slot = makeRect(content.pos.x - side * 0.1f, (size().y - side) / 2, side, side);
		painter.image(slot, mEntry.icon);
	}
	if (checked) {
		const halnf unit = style.metrics.indicatorSize;
		const Vec2F origin = { content.pos.x, (size().y - unit) / 2 };
		painter.polyline(
			{ origin + Vec2F(unit * 0.2f, unit * 0.52f),
				origin + Vec2F(unit * 0.42f, unit * 0.74f),
				origin + Vec2F(unit * 0.82f, unit * 0.28f) },
			textColor,
			1.5f
		);
	}

	const halnf textLeft = content.pos.x + style.metrics.indicatorSize + style.metrics.spacing;
	const RectF textBox = makeRect(textLeft, 0, size().x - textLeft, size().y);
	painter.text(mText, textBox, style.metrics.fontSize, TextAlign::Start, textColor);

	if (!mEntry.shortcut.empty()) {
		const halnf right = mEntry.kind == MenuEntry::Kind::Submenu ? kSubmenuArrowWidth : 0;
		const RectF shortcutBox = makeRect(content.pos.x, 0, content.size.x - right, size().y);
		painter.text(mEntry.shortcut, shortcutBox, style.metrics.smallFontSize, TextAlign::End, style.palette.mutedText);
	}

	if (mEntry.kind == MenuEntry::Kind::Submenu) {
		const Vec2F tip = { size().x - 10, size().y / 2 };
		painter.polyline({ tip + Vec2F(-3.0f, -4.0f), tip, tip + Vec2F(-3.0f, 4.0f) }, style.palette.mutedText, 1.5f);
	}
}

void MenuItem::onActivated() {
	if (mEntry.kind == MenuEntry::Kind::Submenu) {
		openSubmenu();
		return;
	}

	if (mEntry.action) mEntry.action();
	mMenu.closeChain();
	if (mMenu.mOnItemActivated) mMenu.mOnItemActivated();
}

void MenuItem::onHoverChanged() {
	Control::onHoverChanged();
	if (!isHovered()) return;

	if (mEntry.kind == MenuEntry::Kind::Submenu) openSubmenu();
	else if (ui()) {
		for (const auto& sibling : mMenu.items()) {
			const MenuEntry& siblingEntry = sibling->entry();
			if (siblingEntry.kind == MenuEntry::Kind::Submenu && siblingEntry.submenu->isOpen()) {
				siblingEntry.submenu->closeMenu();
			}
		}
	}
}

RGBA MenuItem::backgroundTarget() const {
	const Palette& palette = theme().palette;
	const bool submenuOpen = mEntry.kind == MenuEntry::Kind::Submenu && mEntry.submenu->isOpen();

	if (isEffectivelyEnabled() && (isHovered() || submenuOpen)) return palette.menuHighlight;
	return { palette.menuHighlight.r, palette.menuHighlight.g, palette.menuHighlight.b, 0 };
}

Placement MenuItem::submenuSide(const RectF& menuRect) const {
	const RectF* owner = mMenu.ui() ? mMenu.ui()->popupAnchorOf(mMenu) : nullptr;
	if (!owner) return Placement::Right;
	const bool openedLeftOfOwner = menuRect.pos.x + menuRect.size.x <= owner->pos.x + 1;
	return openedLeftOfOwner ? Placement::Left : Placement::Right;
}

void MenuItem::openSubmenu() {
	if (!mEntry.submenu || mEntry.submenu->isOpen()) return;

	const RectF menuRect = mMenu.windowRect();
	const RectF itemRect = windowRect();
	const RectF anchor = makeRect(menuRect.pos.x, itemRect.pos.y, menuRect.size.x, itemRect.size.y);

	mEntry.submenu->openAt(*this, anchor, submenuSide(menuRect));
	refreshVisualState();
}

Menu::Menu() :
	Panel(Surface::Raised) {
	setDebugName("menu");
	setBorder(true);
	setCornerRadius(Theme::standard().metrics.popupCornerRadius);
	emplaceLayout<ColumnLayout>(0);

	mColumn.setDebugName("menu items");
	mColumn.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mColumn.emplaceLayout<ColumnLayout>(0).setPadding(Theme::standard().metrics.menuPadding);
	mScroll.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mScroll.setContent(&mColumn);
	addChild(mScroll);
}

Menu::~Menu() {
	if (isOpen()) closeMenu();
	mColumn.removeAllChildren();
	mScroll.setContent(nullptr);
	removeAllChildren();
}

MenuItem& Menu::add(MenuEntry entry) {
	mItems.push_back(std::make_unique<MenuItem>(*this, std::move(entry)));
	mColumn.addChild(*mItems.back());
	return *mItems.back();
}

MenuItem& Menu::addItem(std::string text, std::function<void()> action, std::string shortcut) {
	return add(MenuEntry::item(std::move(text), std::move(action), std::move(shortcut)));
}

MenuItem& Menu::addToggle(std::string text, std::function<bool()> isChecked, std::function<void()> action) {
	return add(MenuEntry::toggle(std::move(text), std::move(isChecked), std::move(action)));
}

void Menu::addSeparator() {
	mSeparators.push_back(std::make_unique<Separator>());
	mColumn.addChild(*mSeparators.back());
}

MenuItem& Menu::addSubmenu(std::string text, Menu& submenu) { return add(MenuEntry::nested(std::move(text), submenu)); }

void Menu::clear() {
	mColumn.removeAllChildren();
	mItems.clear();
	mSeparators.clear();
}

std::vector<const MenuItem*> Menu::itemsWithSeparators() const {
	std::vector<const MenuItem*> order;
	for (Widget* child : mColumn.children()) {
		const auto* item = dynamic_cast<const MenuItem*>(child);
		if (item) order.push_back(item);
		else if (!order.empty() && order.back()) order.push_back(nullptr);
	}
	while (!order.empty() && !order.back())
		order.pop_back();
	return order;
}

void Menu::refresh() {
	if (mOnOpen) mOnOpen(*this);
	for (const auto& item : mItems)
		item->refresh();
}

void Menu::openAt(Widget& owner, const RectF& anchor, Placement placement) {
	if (!owner.ui()) return;
	refresh();

	PopupRequest request;
	request.content = this;
	request.owner = &owner;
	request.anchor = anchor;
	request.options = menuPopupOptions(placement, owner.theme().metrics.popupGap);
	owner.ui()->openPopup(request);
}

void Menu::closeMenu() {
	if (ui()) ui()->closePopup(*this);
}

bool Menu::isOpen() const { return ui() && ui()->isPopupOpen(*this); }

void Menu::closeChain() {
	if (ui()) ui()->closePopupChain(*this);
}

namespace {
	constexpr halnf kCenterMargin = 3;
}

class MenuBar::MenuBarLayout : public RowLayout {
public:
	explicit MenuBarLayout(MenuBar& bar) :
		RowLayout(2),
		mBar(bar) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		if (!mBar.mCenter) return RowLayout::measureChildren(context, axis, crossLength);

		const Extent row = RowLayout::measureChildren(context.without(*mBar.mCenter), axis, crossLength);
		Extent center = context.measure(*mBar.mCenter, axis, crossLength);
		if (axis == Axis::Vertical) {
			center.minimum += kCenterMargin * 2;
			center.preferred += kCenterMargin * 2;
		}
		if (axis == Axis::Vertical)
			return {
				std::max(row.minimum, center.minimum), std::max(row.preferred, center.preferred), row.maximum, row.grow
			};
		return { row.minimum + center.minimum, row.preferred + center.preferred, row.maximum, row.grow };
	}

	void arrangeChildren(LayoutContext& context, const RectF& contentArea) override {
		mBar.showTitles(fittingTitles(context, contentArea));
		if (!mBar.mCenter) {
			RowLayout::arrangeChildren(context, contentArea);
			return;
		}
		LayoutContext row = context.without(*mBar.mCenter);
		RowLayout::arrangeChildren(row, contentArea);
		placeCenter(context, contentArea);
	}

	void placeCenter(LayoutContext& context, const RectF& area) const {
		Widget& center = *mBar.mCenter;
		const RectF space = mBar.mDragSpace.rect();
		const halnf height =
			std::min(area.size.y - kCenterMargin * 2, context.measure(center, Axis::Vertical, space.size.x).preferred);
		const halnf width = std::min(space.size.x, context.measure(center, Axis::Horizontal, height).preferred);
		const halnf barCenter = area.pos.x + area.size.x / 2;
		const halnf left = std::clamp(barCenter - width / 2, space.pos.x, space.pos.x + space.size.x - width);
		context.place(center, makeRect(left, area.pos.y + (area.size.y - height) / 2, width, height));
	}

private:
	[[nodiscard]] halnf widthOf(const LayoutContext& context, const Widget& widget, halnf height) const {
		return context.measure(widget, Axis::Horizontal, height).preferred;
	}

	[[nodiscard]] size_t fittingTitles(const LayoutContext& context, const RectF& area) const {
		const halnf height = area.size.y;
		halnf controls = mBar.mWindowControls.isVisible() ? widthOf(context, mBar.mWindowControls, height) : 0;
		if (mBar.mTrailing && mBar.mTrailing->isVisible()) controls += widthOf(context, *mBar.mTrailing, height) + gap();
		halnf total = controls + centerWidth(context, height);
		for (const auto& title : mBar.mTitles)
			total += widthOf(context, *title, height) + gap();
		if (total <= area.size.x) return mBar.mTitles.size();

		halnf used = controls + centerWidth(context, height) + widthOf(context, mBar.mOverflow, height) + gap();
		size_t count = 0;
		for (const auto& title : mBar.mTitles) {
			used += widthOf(context, *title, height) + gap();
			if (used > area.size.x) break;
			count++;
		}
		return count;
	}

	[[nodiscard]] halnf centerWidth(const LayoutContext& context, halnf height) const {
		if (!mBar.mCenter) return 0;
		return context.measure(*mBar.mCenter, Axis::Horizontal, height).minimum + gap();
	}

	MenuBar& mBar;
};

MenuBar::MenuBar() :
	Panel(Surface::None) {
	setDebugName("menu bar");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<MenuBarLayout>(*this).setPadding(Insets::symmetric(4, 2));

	mDragSpace.setDebugName("menu bar drag space");
	mDragSpace.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mOverflow.setDebugName("menu bar overflow");
	mOverflow.setStyle(Button::Style::Flat);
	mOverflow.setPopupContent(&mOverflowMenu);
	mOverflow.setPopupOptions(menuPopupOptions(Placement::Below, Theme::standard().metrics.popupGap));
	mOverflow.setOnOpening([this]() { mOverflowMenu.refresh(); });
	mOverflow.setVisible(false);
	mOverflowMenu.setOnItemActivated([this]() { mOverflow.closePopup(); });
	addChild(mOverflow);
	addChild(mDragSpace);
	addChild(mWindowControls);
}

void MenuBar::showTitles(size_t count) {
	if (count == mShownTitles && mOverflow.isVisible() == (count < mTitles.size())) return;
	mShownTitles = count;
	for (size_t index = 0; index < mTitles.size(); index++)
		mTitles[index]->setVisible(index < count);
	mOverflow.setVisible(count < mTitles.size());
	mOverflowMenu.clear();
	for (size_t index = count; index < mTitles.size(); index++)
		mOverflowMenu.addSubmenu(mTitles[index]->text(), *mMenus[index]);
}

void MenuBar::onEvent(Event& event) {
	WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	if (!host || !host->platformInfo().canMoveWindow) return;

	if (mMovePending) {
		handlePendingMove(event, *host);
		return;
	}

	if (event.type() == Event::Type::PointerDown && isDragSurface(event.target())) handleDragSurfacePress(event, *host);
}

void MenuBar::handleDragSurfacePress(Event& event, WindowHost& host) {
	if (event.button() == PointerButton::Secondary) {
		host.showWindowMenu(event.windowPosition());
		event.accept();
		return;
	}

	if (event.button() != PointerButton::Primary) return;
	event.accept();

	if (event.clickCount() == 2) {
		host.toggleMaximizeWindow();
		return;
	}

	mMovePending = true;
	mMovePressPoint = event.windowPosition();
	ui()->capturePointer(*this);
}

void MenuBar::handlePendingMove(Event& event, WindowHost& host) {
	switch (event.type()) {
		case Event::Type::PointerMove:
			{
				const Vec2F offset = event.windowPosition() - mMovePressPoint;
				if (offset.x * offset.x + offset.y * offset.y < kWindowDragThreshold * kWindowDragThreshold) return;

				cancelPendingMove();
				host.beginWindowMove();
				event.accept();
				return;
			}

		case Event::Type::PointerUp:
			cancelPendingMove();
			event.accept();
			return;

		default: return;
	}
}

void MenuBar::cancelPendingMove() {
	mMovePending = false;
	if (ui()) ui()->releasePointer(*this);
}

MenuIconButton::MenuIconButton() {
	setDebugName("menu icon button");
	setStyle(Style::Flat);
	setIconSize(16);
}

void MenuIconButton::paintIcon(Painter& painter, const RectF& box) {
	const RGBA color = isHovered() ? theme().palette.text : theme().palette.mutedText;
	const halnf thickness = 1.6f;
	const halnf width = box.size.x * 0.8f;
	const halnf left = box.pos.x + (box.size.x - width) / 2;
	const halnf spacing = box.size.y * 0.3f;
	const halnf middle = box.pos.y + box.size.y / 2;
	for (int row = -1; row <= 1; row++) {
		const halnf y = middle + halnf(row) * spacing;
		painter.line(Vec2F(left, y), Vec2F(left + width, y), color, thickness);
	}
}

bool MenuBar::isDragSurface(const Widget* target) const { return target == this || target == &mDragSpace; }

void MenuBar::setCenterWidget(Widget* widget) {
	if (mCenter) removeChild(*mCenter);
	mCenter = widget;
	if (widget) addChild(*widget);
	requestLayout();
}

PopupButton& MenuBar::addMenu(std::string title, Menu& menu) {
	return insertMenu(mTitles.size() - mTrailingMenus, std::move(title), menu);
}

PopupButton& MenuBar::addTrailingMenu(std::string title, Menu& menu) {
	PopupButton& button = insertMenu(mTitles.size(), std::move(title), menu);
	mTrailingMenus++;
	return button;
}

void MenuBar::removeMenu(Menu& menu) {
	const auto found = std::find(mMenus.begin(), mMenus.end(), &menu);
	if (found == mMenus.end()) return;

	const size_t index = (size_t) (found - mMenus.begin());
	if (index >= mTitles.size() - mTrailingMenus) mTrailingMenus--;
	menu.setOnItemActivated({});
	mTitles[index]->closePopup();
	removeChild(*mTitles[index]);
	mTitles.erase(mTitles.begin() + (std::ptrdiff_t) index);
	mMenus.erase(found);
	mShownTitles = mTitles.size();
	mOverflowMenu.clear();
	requestLayout();
}

void MenuBar::setTrailingWidget(Widget* widget) {
	if (widget == mTrailing) return;
	if (mTrailing) removeChild(*mTrailing);
	mTrailing = widget;
	if (widget) insertChild(children().size() - 1, *widget);
	requestLayout();
}

PopupButton& MenuBar::insertMenu(size_t index, std::string title, Menu& menu) {
	mTitles.insert(mTitles.begin() + (std::ptrdiff_t) index, std::make_unique<PopupButton>(std::move(title)));
	PopupButton& button = *mTitles[index];

	button.setStyle(Button::Style::Flat);
	button.setPopupContent(&menu);
	button.setPopupOptions(menuPopupOptions(Placement::Below, Theme::standard().metrics.popupGap));
	button.setOnHovered([this, &button]() { onTitleHovered(button); });
	button.setOnOpening([&menu]() { menu.refresh(); });
	menu.setOnItemActivated([this, &button]() {
		button.closePopup();
		mOverflow.closePopup();
	});

	mMenus.insert(mMenus.begin() + (std::ptrdiff_t) index, &menu);
	insertChild(index, button);
	mShownTitles = mTitles.size();
	return button;
}

void MenuBar::onTitleHovered(PopupButton& title) {
	for (const auto& other : mTitles) {
		if (other.get() == &title || !other->isPopupOpen()) continue;

		other->closePopup();
		title.openPopupFromHover();
		return;
	}
}

Dropdown::Dropdown(std::vector<std::string> options) {
	setDebugName("dropdown");
	setPopupContent(&mMenu);
	setPopupOptions(menuPopupOptions(Placement::Below, Theme::standard().metrics.popupGap));
	setOptions(std::move(options));
}

void Dropdown::setOptions(std::vector<std::string> options) {
	mOptions = std::move(options);
	if (mSelected >= (long) mOptions.size()) mSelected = -1;
	if (mSelected < 0 && !mOptions.empty()) mSelected = 0;

	rebuildMenu();
	setText(selectedText());
}

void Dropdown::setSelectedIndex(long index) {
	if (index < -1 || index >= (long) mOptions.size() || index == mSelected) return;

	mSelected = index;
	setText(selectedText());
	if (mOnSelected) mOnSelected(mSelected);
}

std::string Dropdown::selectedText() const { return mSelected >= 0 ? mOptions[(size_t) mSelected] : std::string(); }

Extent Dropdown::measureContent(Axis axis, halnf crossLength) const {
	Extent extent = Button::measureContent(axis, crossLength);
	if (axis != Axis::Horizontal) return extent;

	halnf widest = 0;
	for (const std::string& option : mOptions) {
		const halnf width = textWidth(option, theme().metrics.fontSize);
		if (width > widest) widest = width;
	}

	extent.preferred = widest + theme().metrics.controlPadding.total(Axis::Horizontal) + kSubmenuArrowWidth;
	return extent;
}

void Dropdown::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF content = style.metrics.controlPadding.shrink(localRect());

	const halnf radius = style.metrics.cornerRadius;
	const RGBA border = isPopupOpen() || showsFocusRing() ? style.palette.mutedText : style.palette.controlBorder;
	painter.fillRect(localRect(), background(), radius);
	painter.strokeRect(localRect(), border, radius);

	const RectF textBox = makeRect(content.pos, { content.size.x - kSubmenuArrowWidth, content.size.y });
	painter.text(selectedText(), textBox, style.metrics.fontSize, TextAlign::Start, style.palette.text);

	const Vec2F chevron = { content.pos.x + content.size.x - kSubmenuArrowWidth / 2, size().y / 2 + 1.5f };
	painter.polyline(
		{ chevron + Vec2F(-4.0f, -2.5f), chevron + Vec2F(0.0f, 1.5f), chevron + Vec2F(4.0f, -2.5f) },
		style.palette.mutedText,
		1.5f
	);
}

void Dropdown::onActivated() {
	mMenu.setSizing(AxisSizing::fit().atLeast(size().x), AxisSizing::fit());
	PopupButton::onActivated();
}

void Dropdown::rebuildMenu() {
	mMenu.clear();

	for (size_t index = 0; index < mOptions.size(); index++) {
		const long optionIndex = (long) index;
		mMenu.addToggle(
			mOptions[index],
			[this, optionIndex]() { return mSelected == optionIndex; },
			[this, optionIndex]() { setSelectedIndex(optionIndex); }
		);
	}

	mMenu.setOnItemActivated([this]() { closePopup(); });
}

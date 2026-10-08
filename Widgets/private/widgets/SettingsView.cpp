#include "SettingsView.hpp"

#include "BoxLayout.hpp"
#include "Painter.hpp"
#include "StackLayout.hpp"
#include "Ui.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kRowHeightFactor = 1.6f;
	constexpr halnf kChevronWidth = 24;
	constexpr halnf kPageWidth = 760;
	constexpr halnf kSwitchWidthFactor = 1.9f;
	constexpr halnf kPagePadding = 16;

	class SettingsSection : public Panel {
	public:
		SettingsSection() :
			Panel(Surface::Raised) {
			setDebugName("settings section");
			setSizing(AxisSizing::fill(), AxisSizing::fit());
			setCornerRadius(Theme::standard().metrics.popupCornerRadius);
			emplaceLayout<ColumnLayout>(0).setPadding(Insets::uniform(4));
		}
	};
}

SettingsRow::SettingsRow(SettingsView& view, Menu& menu, const MenuItem& item) :
	mView(view),
	mMenu(menu),
	mEntry(item.entry()) {
	setDebugName("settings row: " + mEntry.text);
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	refresh();
}

void SettingsRow::refresh() {
	if (mEntry.isEnabled) setEnabled(mEntry.isEnabled());
	std::string text = mEntry.dynamicText ? mEntry.dynamicText() : mEntry.text;
	if (text != mText) {
		mText = std::move(text);
		requestLayout();
	}
	requestPaint();
}

Extent SettingsRow::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;
	if (axis == Axis::Vertical) return Extent::rigid(metrics.menuItemHeight * kRowHeightFactor);
	return Extent::natural(
		metrics.controlPadding.total(Axis::Horizontal) + textWidth(mText, metrics.fontSize) + kChevronWidth * 2
	);
}

void SettingsRow::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF content = style.metrics.controlPadding.shrink(localRect());
	RGBA textColor = isEffectivelyEnabled() ? style.palette.text : style.palette.disabledText;
	if (mEntry.muted && isEffectivelyEnabled()) textColor = style.palette.mutedText;

	painter.fillRect(localRect(), background(), style.metrics.cornerRadius);
	painter.text(
		mText, makeRect(content.pos.x, 0, content.size.x, size().y), style.metrics.fontSize, TextAlign::Start, textColor
	);

	const halnf middle = size().y / 2;
	const halnf right = content.pos.x + content.size.x;
	if (mEntry.kind == MenuEntry::Kind::Submenu) {
		const Vec2F tip = { right - 4, middle };
		painter.polyline({ tip + Vec2F(-5.0f, -7.0f), tip, tip + Vec2F(-5.0f, 7.0f) }, style.palette.mutedText, 2);
		return;
	}

	if (mEntry.kind == MenuEntry::Kind::Toggle) {
		const bool on = mEntry.isChecked && mEntry.isChecked();
		const halnf height = style.metrics.indicatorSize;
		const halnf width = height * kSwitchWidthFactor;
		const RectF track = makeRect(right - width, middle - height / 2, width, height);
		painter.fillRect(track, on ? style.palette.toggleOn : style.palette.track, height / 2);
		const halnf knob = height / 2 - 3;
		const Vec2F center = { on ? track.pos.x + width - height / 2 : track.pos.x + height / 2, middle };
		painter.fillCircle(center, knob, on ? style.palette.onToggle : style.palette.mutedText);
		return;
	}

	if (!mEntry.shortcut.empty()) {
		painter.text(
			mEntry.shortcut,
			makeRect(content.pos.x, 0, content.size.x, size().y),
			style.metrics.smallFontSize,
			TextAlign::End,
			style.palette.mutedText
		);
	}
}

void SettingsRow::onActivated() { mView.activated(mMenu, mEntry); }

SettingsPage::SettingsPage(SettingsView& view, std::string title) :
	StackPage(view, std::move(title)),
	mView(view) {
	setDebugName("settings page: " + this->title());
}

SettingsRow* SettingsPage::row(const std::string& text) const {
	for (const auto& row : mRows)
		if (row->text() == text) return row.get();
	return nullptr;
}

void SettingsPage::clear() {
	clearContent();
	for (const auto& section : mSections)
		section->removeAllChildren();
	mRows.clear();
	mSections.clear();
	mMenu = nullptr;
}

void SettingsPage::showMenu(Menu& menu) {
	clear();
	mMenu = &menu;
	menu.refresh();

	Widget* section = nullptr;
	for (const MenuItem* item : menu.itemsWithSeparators()) {
		if (!item) {
			section = nullptr;
			continue;
		}
		if (!section) {
			mSections.push_back(std::make_unique<SettingsSection>());
			section = mSections.back().get();
			body().addChild(*section);
		}
		mRows.push_back(std::make_unique<SettingsRow>(mView, menu, *item));
		section->addChild(*mRows.back());
	}
}

void SettingsPage::showContent(Widget& content) {
	clear();
	body().addChild(content);
}

void SettingsPage::refresh() {
	if (mMenu) showMenu(*mMenu);
}

void SettingsPage::refreshRows() {
	for (const auto& row : mRows)
		row->refresh();
}

SettingsView::SettingsView() :
	PageStack(Surface::Window) {
	setDebugName("settings view");
	setPagePadding(Insets::uniform(kPagePadding));
	setPageMaximumWidth(kPageWidth);
	setRootBackText("Close");
	setOnBackFromRoot([this]() { close(); });
}

SettingsView::~SettingsView() {
	if (isOpen()) close();
}

void SettingsView::setRoot(std::string title, Menu& menu) {
	mRootTitle = std::move(title);
	mRootMenu = &menu;
	mRootContent = nullptr;
}

void SettingsView::setRootContent(std::string title, Widget& content) {
	mRootTitle = std::move(title);
	mRootContent = &content;
	mRootMenu = nullptr;
}

void SettingsView::setOnPageShown(std::function<void(SettingsPage&)> onPageShown) {
	PageStack::setOnPageShown([onPageShown = std::move(onPageShown)](StackPage& page) {
		if (onPageShown) onPageShown(static_cast<SettingsPage&>(page));
	});
}

std::unique_ptr<StackPage> SettingsView::makePage(std::string title) {
	return std::make_unique<SettingsPage>(*this, std::move(title));
}

void SettingsView::open(Ui& ui) {
	clear();
	if (mRootMenu) openMenu(mRootTitle, *mRootMenu);
	if (mRootContent) openContent(mRootTitle, *mRootContent);

	const RectF viewport = ui.viewport();
	setSizing(AxisSizing::fixed(viewport.size.x), AxisSizing::fixed(viewport.size.y));
	PopupRequest request;
	request.content = this;
	request.anchor = viewport;
	request.options.placement = Placement::Over;
	request.options.modal = true;
	request.options.closeOnOutsidePress = false;
	request.options.closeOnEscape = false;
	request.options.appearFrom =
		makeRect(viewport.pos.x, viewport.pos.y + viewport.size.y, viewport.size.x, viewport.size.y);
	request.options.onClosed = [this]() {
		if (mOnClosed) mOnClosed();
	};
	ui.openPopup(request);
}

void SettingsView::close() {
	if (ui()) ui()->closePopup(*this);
}

bool SettingsView::isOpen() const { return ui() && ui()->isPopupOpen(*this); }

void SettingsView::openMenu(std::string title, Menu& menu) {
	static_cast<SettingsPage&>(push(std::move(title))).showMenu(menu);
}

void SettingsView::openContent(std::string title, Widget& content) {
	static_cast<SettingsPage&>(push(std::move(title))).showContent(content);
}

void SettingsView::activated(Menu& menu, const MenuEntry& entry) {
	if (entry.kind == MenuEntry::Kind::Submenu) {
		if (entry.submenu) openMenu(entry.dynamicText ? entry.dynamicText() : entry.text, *entry.submenu);
		return;
	}

	if (entry.kind == MenuEntry::Kind::Action && mOnContentRequested && mOnContentRequested(entry)) return;

	const bool toggle = entry.kind == MenuEntry::Kind::Toggle;
	const auto action = entry.action;
	if (!toggle) close();
	if (action) action();
	if (toggle) {
		if (depth() > 0) currentPage().refreshRows();
		return;
	}
	menu.notifyItemActivated();
}

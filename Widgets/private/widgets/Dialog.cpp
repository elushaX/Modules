#include "Dialog.hpp"

#include "BoxLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kDialogWidth = 360;
}

Dialog::Dialog(std::string title) :
	Panel(Surface::Raised),
	mTitle(std::move(title)) {
	setDebugName("dialog");
	setBorder(true);
	setSizing(AxisSizing::fixed(kDialogWidth), AxisSizing::fit());
	emplaceLayout<ColumnLayout>(10).setPadding(Insets::uniform(16));

	mTitle.setFontSize(Theme::standard().metrics.fontSize + 2);
	mTitle.setWrap(true);
	mTitle.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mMessage.setWrap(true);
	mMessage.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mMessage.setTone(Label::Tone::Muted);
	mMessage.setVisible(false);

	mBody.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mBody.emplaceLayout<ColumnLayout>(8);

	mButtonRow.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mButtonRow.emplaceLayout<RowLayout>(6);
	mButtonSpacer.setSizing(AxisSizing::fill(), AxisSizing::fixed(1));
	mButtonRow.addChild(mButtonSpacer);

	addChild(mTitle);
	addChild(mMessage);
	addChild(mBody);
	addChild(mButtonRow);
}

Dialog::~Dialog() { close(); }

void Dialog::setTitle(std::string title) { mTitle.setText(std::move(title)); }

void Dialog::setMessage(std::string message) {
	mMessage.setVisible(!message.empty());
	mMessage.setText(std::move(message));
}

Button& Dialog::addButton(std::string text, std::function<void()> action, bool isDefault) {
	auto button = std::make_unique<Button>(std::move(text));
	button->setOnActivate([this, action]() {
		close();
		if (action) action();
	});
	if (isDefault) {
		button->setStyle(Button::Style::Accent);
		mDefault = button.get();
	}

	mButtonRow.addChild(*button);
	mButtons.push_back(std::move(button));
	return *mButtons.back();
}

Button& Dialog::addCancelButton(std::string text) {
	Button& button = addButton(std::move(text), {});
	button.setOnActivate([this]() { cancel(); });
	return button;
}

void Dialog::open(Ui& ui, Widget* owner) {
	const RectF viewport = ui.viewport();

	PopupRequest request;
	request.content = this;
	request.owner = owner;
	request.anchor = makeRect(viewport.pos.x, viewport.pos.y + viewport.size.y / 4, viewport.size.x, 0);
	request.options.placement = Placement::Over;
	request.options.align = PlacementAlign::Center;
	request.options.modal = true;
	request.options.closeOnOutsidePress = false;
	request.options.onClosed = [this]() {
		if (mOnCancel) mOnCancel();
	};
	ui.openPopup(request);
	if (mDefault) ui.setKeyboardFocus(mDefault);
}

void Dialog::close() {
	if (!ui() || !ui()->isPopupOpen(*this)) return;

	const std::function<void()> onCancel = std::move(mOnCancel);
	ui()->closePopup(*this);
	mOnCancel = onCancel;
}

bool Dialog::isOpen() const { return ui() && ui()->isPopupOpen(*this); }

void Dialog::accept() {
	if (mDefault) mDefault->activate();
}

void Dialog::cancel() {
	if (!isOpen()) return;
	ui()->closePopup(*this);
}

void Dialog::onEvent(Event& event) {
	if (event.type() != Event::Type::KeyDown) return;

	if (event.keyCode() == InputID::ENTER || event.keyCode() == InputID::KP_ENTER) {
		accept();
		event.accept();
	}
}

ConfirmDialog::ConfirmDialog() {
	setDebugName("confirm dialog");
	mConfirm = &addButton(
		{},
		[this]() {
			if (mOnConfirm) mOnConfirm();
		},
		true
	);
	addCancelButton();
}

void ConfirmDialog::ask(
	Ui& ui, std::string title, std::string message, std::string confirmText, std::function<void()> onConfirm
) {
	setTitle(std::move(title));
	setMessage(std::move(message));
	mConfirm->setText(std::move(confirmText));
	mOnConfirm = std::move(onConfirm);
	open(ui);
}

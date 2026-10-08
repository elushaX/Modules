#include "Notice.hpp"

#include "Ui.hpp"

using namespace tp;
using namespace tp::ui;

Notice::Notice() { setDebugName("notice"); }

Notice::~Notice() { hide(); }

void Notice::show(Widget& anchor, std::string text, halnf durationMs) {
	Ui* ui = anchor.ui();
	if (!ui) return;

	setText(std::move(text));
	mRemainingMs = durationMs;

	if (!ui->isPopupOpen(*this)) {
		const RectF area = anchor.windowRect();
		PopupRequest request;
		request.content = this;
		request.anchor = makeRect(area.pos.x, area.pos.y + area.size.y / 3, area.size.x, 0);
		request.options.placement = Placement::Over;
		request.options.align = PlacementAlign::Center;
		request.options.interactive = false;
		request.options.closeOnOutsidePress = false;
		request.options.closeOnEscape = false;
		ui->openPopup(request);
	}

	startTicking();
	requestPaint();
}

void Notice::hide() {
	mRemainingMs = 0;
	if (ui() && ui()->isPopupOpen(*this)) ui()->closePopup(*this);
}

bool Notice::isShown() const { return ui() && ui()->isPopupOpen(*this); }

void Notice::onTick(halnf deltaMs) {
	mRemainingMs -= deltaMs;
	if (mRemainingMs > 0) return;

	stopTicking();
	hide();
}

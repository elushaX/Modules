#include "TaskWidget.hpp"

#include "BoxLayout.hpp"
#include "StackLayout.hpp"

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kPadding = 12;
	constexpr halnf kRowGap = 6;
	constexpr halnf kBodyInset = 6;
	constexpr halnf kPageMaximumWidth = 920;
}

AppStorage tp::taskboard::taskboardStorage() { return AppStorage::inHome("taskboard"); }

TaskWidget::TaskWidget(TaskBoard& board, FileSystem& files, const AppStorage& storage) :
	mPresets(files, storage.root()),
	description(board),
	workspace(board, files),
	session(board, mPresets),
	page(board) {
	setDebugName("task widget");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	emplaceLayout<ColumnLayout>(kRowGap);

	buildSessionPage();
	buildTaskPage();

	carousel.setSizing(AxisSizing::fill(), AxisSizing::fill());
	carousel.addTab("Task", description);
	carousel.addTab("Session", sessionScroll);
	carousel.addTab("Workdir", workspace);
	carousel.addTab("Links", taskPages);
	description.header.badgeRow().setSizing(AxisSizing::fill(), AxisSizing::fit());
	mBody.setDebugName("task body");
	mBody.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mBody.setCornerRadius(theme().metrics.cornerRadius);
	mBody.emplaceLayout<StackLayout>().setPadding(Insets::uniform(kBodyInset));
	mBody.addChild(carousel);
	addChild(description.header.badgeRow());
	addChild(mBody);

	showTask(std::nullopt);
}

void TaskWidget::buildSessionPage() {
	sessionHint.setTone(Label::Tone::Muted);
	sessionHint.setWrap(true);
	sessionHint.setSizing(AxisSizing::fill(), AxisSizing::fit());

	mSessionBody.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mSessionBody.emplaceLayout<StackLayout>().setPadding(Insets::uniform(kPadding));
	mSessionBody.addChild(session);
	mSessionBody.addChild(sessionHint);

	sessionScroll.setSizing(AxisSizing::fill(), AxisSizing::fill());
	sessionScroll.setContent(&mSessionBody);
}

void TaskWidget::buildTaskPage() {
	taskPages.setSizing(AxisSizing::fill(), AxisSizing::fill());
	taskPages.setPagePadding(Insets::uniform(kPadding));
	taskPages.setPageHeadersShown(false);
	taskPages.setPageMaximumWidth(kPageMaximumWidth);
}

void TaskWidget::showTask(std::optional<NodeId> id) {
	const bool hasTask = id.has_value();
	page.showTask(id);
	description.showTask(id);
	workspace.showTask(id);
	session.showTask(id);
	session.setVisible(hasTask);
	sessionHint.setVisible(!hasTask);
}

void TaskWidget::setReadOnly(bool readOnly) {
	page.setEnabled(!readOnly);
	session.setEnabled(!readOnly);
	description.header.setEnabled(!readOnly);
	description.editor.sourceArea().setEnabled(!readOnly);
}

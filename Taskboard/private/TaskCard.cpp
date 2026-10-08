#include "TaskCard.hpp"

#include "BoxLayout.hpp"
#include "Painter.hpp"
#include "TaskAppearance.hpp"
#include "Theme.hpp"

#include <cmath>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kHeaderGap = 6;
	constexpr halnf kBodyGap = 4;
	constexpr halnf kTitleFontSize = 16;
	constexpr const char* kNoDescription = "No description";

}

TaskCardView::TaskCardView() {
	setDebugName("task card view");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	setClipsChildren(true);
	emplaceLayout<ColumnLayout>(kBodyGap);

	status.setDotShown(false);
	number.setTone(Label::Tone::Muted);
	number.setSizing(AxisSizing::fit(), AxisSizing::fit());
	mHeaderSpacer.setSizing(AxisSizing::fill(), AxisSizing::fixed(1));
	mHeader.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mHeader.emplaceLayout<RowLayout>(kHeaderGap).setCrossAlign(CrossAlign::Center);
	mHeader.addChild(number);
	mHeader.addChild(mHeaderSpacer);
	mHeader.addChild(status);
	mHeader.addChild(session);

	title.setWrap(true);
	title.setFontSize(kTitleFontSize);
	title.setMaximumLines(2);
	title.setSizing(AxisSizing::fill(), AxisSizing::fit());

	description.setWrap(true);
	description.setElidesToHeight(true);
	description.setTone(Label::Tone::Muted);
	description.setSizing(AxisSizing::fill(), AxisSizing::fit());

	addChild(mHeader);
	addChild(title);
	addChild(description);
}

void TaskCardView::setStatusShown(bool shown) {
	mStatusShown = shown;
	status.setVisible(shown);
}

void TaskCardView::showTask(NodeId id, const Task& task) {
	number.setText("#" + std::to_string(id));
	status.show(std::string(statusLabel(task.status)), taskStatusColor(task.status), false);
	status.setVisible(mStatusShown);
	session.showSession(task.session.state);
	title.setText(task.title);
	description.setText(task.description.empty() ? std::string(kNoDescription) : task.description);
}

TaskTreeCard::TaskTreeCard(TreeItemId id) :
	TreeCard(id) {
	setHeaderShown(false);
	content().addChild(mView);
}

void TaskTreeCard::showTask(const Task& task) {
	setTitle(taskLabel(id(), task));
	setLabel("#" + std::to_string(id()));
	setAccent(taskAccentColor(task));
	const SessionState state = task.session.state;
	setBadge(
		state == SessionState::None ? std::nullopt : std::optional<RGBA>(sessionStateColor(state)), isSessionActive(state)
	);
	mView.showTask(id(), task);
}

TaskKanbanCard::TaskKanbanCard(KanbanItemId id) :
	KanbanCard(id) {
	mView.setStatusShown(false);
	setContent(mView);
}

void TaskKanbanCard::showTask(const Task& task) {
	mView.showTask(id(), task);
	setTitle(taskLabel(id(), task));
	setDetail(mView.description.text());
	setAccent(taskAccentColor(task));
}

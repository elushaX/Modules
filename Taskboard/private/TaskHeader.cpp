#include "TaskHeader.hpp"

#include "BoxLayout.hpp"
#include "TaskAppearance.hpp"
#include "TaskSearch.hpp"
#include "Ui.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kGap = 6;
	constexpr halnf kTitleFontSize = 18;

	std::vector<std::string> statusNames() {
		std::vector<std::string> names;
		for (TaskStatus status : kTaskStatuses)
			names.emplace_back(statusLabel(status));
		return names;
	}
}

TaskTitleField::TaskTitleField() {
	setPlaceholder("Task title");
	setFontSize(kTitleFontSize);
	setSizing(AxisSizing::fill(), AxisSizing::fit());
}

void TaskTitleField::showTitle(const std::string& title) {
	if (!isEditing() && text() != title) setText(title);
}

TaskHeader::TaskHeader(TaskBoard& board) :
	mBoard(board) {
	setDebugName("task header");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<ColumnLayout>(kGap);

	buildConflictBanner();
	buildBadgeRow();
	parentPicker.setPlaceholder("New parent: #id or title");
	parentPicker.setVisible(false);

	addChild(mConflictBanner);
	addChild(title);
	addChild(parentPicker);

	bindControls();
	bindActions();
	showTask(std::nullopt);
}

void TaskHeader::buildBadgeRow() {
	mBadgeRow.setDebugName("task badge row");
	mBadgeRow.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mBadgeRow.emplaceLayout<RowLayout>(kGap).setCrossAlign(CrossAlign::Center);

	back.setStyle(Button::Style::Flat);
	back.setTooltip("Back to the overview  (Esc)");
	back.setVisible(false);
	back.setOnActivate([this] {
		if (mOnBack) mOnBack();
	});
	idLabel.setTone(Label::Tone::Muted);
	mBadgeSpacer.setSizing(AxisSizing::fill(), AxisSizing::fixed(1));
	status.setOptions(statusNames());
	status.setSizing(AxisSizing::fit(), AxisSizing::fit());
	color.setPalette(mBoard.colorPalette());
	color.setTooltip("Task color");
	actionsButton.setTooltip("Task actions");
	actionsButton.setPopupContent(&actionsMenu);
	actionsMenu.setOnItemActivated([this] { actionsButton.closePopup(); });

	for (Widget* widget :
			 std::initializer_list<Widget*>{ &back, &idLabel, &mBadgeSpacer, &status, &badge, &color, &actionsButton })
		mBadgeRow.addChild(*widget);
}

void TaskHeader::setOnBack(std::function<void()> callback) {
	mOnBack = std::move(callback);
	back.setVisible(mOnBack != nullptr);
}

void TaskHeader::buildConflictBanner() {
	conflictText.setWrap(true);
	conflictText.setSizing(AxisSizing::fill(), AxisSizing::fit());
	keepMine.setStyle(Button::Style::Accent);
	mConflictActions.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mConflictActions.emplaceLayout<RowLayout>(kGap).setMainAlign(MainAlign::End);
	mConflictActions.addChild(useTheirs);
	mConflictActions.addChild(keepMine);
	mConflictBanner.setBorder(true);
	mConflictBanner.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mConflictBanner.emplaceLayout<ColumnLayout>(kGap).setPadding(Insets::uniform(kGap + 2));
	mConflictBanner.addChild(conflictText);
	mConflictBanner.addChild(mConflictActions);
	mConflictBanner.setVisible(false);
	keepMine.setOnActivate([this] {
		if (mOnConflictChoice) mOnConflictChoice(true);
	});
	useTheirs.setOnActivate([this] {
		if (mOnConflictChoice) mOnConflictChoice(false);
	});
}

void TaskHeader::bindControls() {
	title.setOnEdited([this](const std::string& text) { change([&](Task& task) { task.title = text; }); });
	title.setOnEditingStarted([this] {
		if (mShown && mOnEditingChanged) mOnEditingChanged(std::make_pair(*mShown, TaskField::Title));
	});
	title.setOnCommitted([this](const std::string&) {
		if (mOnEditingChanged) mOnEditingChanged(std::nullopt);
	});
	status.setOnSelected([this](long index) { change([&](Task& task) { task.status = kTaskStatuses[size_t(index)]; }); });
	color.setOnPicked([this](const RGBA& picked) { change([&](Task& task) { task.color = picked; }); });
	color.setOnPaletteChanged([this](const std::vector<RGBA>& colors) {
		mBoard.setColorPalette(colors);
		if (mOnChanged) mOnChanged();
	});
}

void TaskHeader::bindActions() {
	actionsMenu.addItem("Set parent...", [this] { showParentPicker(); });
	actionsMenu.addSeparator();
	actionsMenu.addItem("Delete task...", [this] { askDelete(); });
	parentPicker.setQuery([this](const std::string& query) { return parentResults(query); });
	parentPicker.setOnChosen([this](size_t index) {
		if (index < mParentChoices.size()) parentPicked(mParentChoices[index]);
	});
}

void TaskHeader::showParentPicker() {
	parentPicker.setVisible(true);
	parentPicker.focus();
	parentPicker.setText("");
}

void TaskHeader::askDelete() {
	if (!mShown || !ui()) return;

	const Task* task = mBoard.task(*mShown);
	const size_t children = mBoard.childrenOf(*mShown).size();
	std::string message = "Delete " + taskLabel(*mShown, *task) + "?";
	if (children > 0) message += " Its " + std::to_string(children) + " child task(s) move up to its parent.";
	deleteConfirm.ask(*ui(), "Delete task", message, "Delete", [this] {
		if (mOnDeleteRequested) mOnDeleteRequested();
	});
}

std::vector<SearchResult> TaskHeader::parentResults(const std::string& query) {
	mParentChoices.clear();
	std::vector<SearchResult> results;
	if (!mShown) return results;

	const bool topLevel = !mBoard.parentOf(*mShown);
	mParentChoices.push_back(std::nullopt);
	results.push_back({ "Top level", topLevel ? "current" : "no parent", !topLevel });

	const std::string needle = lowercaseText(query);
	const std::optional<NodeId> wantedId = taskIdQuery(query);
	for (const Node& node : mBoard.graph().nodes()) {
		const Task* task = mBoard.task(node.id);
		if (!task || node.id == *mShown) continue;
		if (!query.empty()) {
			const bool matches =
				wantedId ? node.id == *wantedId : lowercaseText(task->title).find(needle) != std::string::npos;
			if (!matches) continue;
		}

		const bool current = mBoard.parentOf(*mShown) == node.id;
		const bool allowed = mBoard.canSetParent(*mShown, node.id);
		std::string detail = current ? "current" : (allowed ? std::string(statusLabel(task->status)) : "inside this task");
		mParentChoices.push_back(node.id);
		results.push_back({ taskLabel(node.id, *task), std::move(detail), allowed && !current });
	}
	return results;
}

void TaskHeader::parentPicked(std::optional<NodeId> parent) {
	parentPicker.setVisible(false);
	if (!mShown) return;

	if (parent) mBoard.setParent(*mShown, *parent);
	else mBoard.clearParent(*mShown);
	mBoard.placeLastAmongSiblings(*mShown);
	if (mOnChanged) mOnChanged();
	if (mOnParentChanged) mOnParentChanged();
}

void TaskHeader::change(const std::function<void(Task&)>& edit) {
	Task* task = mShown ? mBoard.task(*mShown) : nullptr;
	if (!task || mLoading) return;

	edit(*task);
	if (mOnChanged) mOnChanged();
}

void TaskHeader::showTask(std::optional<NodeId> id) {
	mShown = id;
	const Task* task = id ? mBoard.task(*id) : nullptr;

	mBadgeRow.setVisible(task != nullptr);
	title.setVisible(task != nullptr);
	if (task) refresh(*task);
}

void TaskHeader::refresh(const Task& task) {
	const bool wasLoading = mLoading;
	mLoading = true;
	title.showTitle(task.title);
	status.setSelectedIndex(
		long(std::find(kTaskStatuses.begin(), kTaskStatuses.end(), task.status) - kTaskStatuses.begin())
	);
	if (color.palette() != mBoard.colorPalette()) color.setPalette(mBoard.colorPalette());
	color.setFallbackColor(taskStatusColor(task.status));
	color.setColor(task.color);
	mLoading = wasLoading;

	idLabel.setText("#" + std::to_string(*mShown));
	badge.showSession(task.session.state);
}

void TaskHeader::showConflicts(const std::vector<std::string>& summaries) {
	mConflictBanner.setVisible(!summaries.empty());
	if (summaries.empty()) return;

	std::string text = "Changed elsewhere while you edited:";
	for (const std::string& summary : summaries)
		text += "\n" + summary;
	conflictText.setText(text);
}

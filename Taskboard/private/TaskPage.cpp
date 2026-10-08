#include "TaskPage.hpp"

#include "BoxLayout.hpp"
#include "Painter.hpp"
#include "TaskSearch.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <unordered_set>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kGap = 6;
	constexpr halnf kSectionGap = 14;

	Widget& rowLayout(Widget& widget, halnf gap) {
		widget.setSizing(AxisSizing::fill(), AxisSizing::fit());
		widget.emplaceLayout<RowLayout>(gap).setCrossAlign(CrossAlign::Center);
		return widget;
	}

	Widget& columnLayout(Widget& widget, halnf gap) {
		widget.setSizing(AxisSizing::fill(), AxisSizing::fit());
		widget.emplaceLayout<ColumnLayout>(gap);
		return widget;
	}
}

void TaskRowButton::setSwatch(const RGBA& color) {
	mSwatch = color;
	requestPaint();
}

void TaskRowButton::paintIcon(Painter& painter, const RectF& box) {
	const halnf side = std::min(box.size.x, box.size.y) * 0.6f;
	const Vec2F center = box.pos + box.size / 2.0f;
	painter.fillRect(makeRect(center.x - side / 2, center.y - side / 2, side, side), mSwatch, 3);
}

TaskRow::TaskRow(TaskBoard& board, NodeId id, bool removable) :
	mBoard(board),
	mId(id) {
	setDebugName("task row #" + std::to_string(id));
	rowLayout(*this, kGap);

	mOpen.setStyle(Button::Style::Flat);
	mOpen.setIconPlacement(Button::IconPlacement::Leading);
	mOpen.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mOpen.setOnActivate([this] {
		if (mOnOpened) mOnOpened(mId);
	});
	mOpen.setOnClicked([this](const Event& event) {
		const bool doubleClick = event.clickCount() >= 2 && event.clickCount() % 2 == 0;
		if (doubleClick) {
			if (mOnOpened) mOnOpened(mId);
		} else if (mOnPreviewed) mOnPreviewed(mId);
	});
	mRemove.setStyle(Button::Style::Flat);
	mRemove.setTooltip("Remove link");
	mRemove.setVisible(removable);
	mRemove.setOnActivate([this] {
		if (mOnRemoved) mOnRemoved(mId);
	});

	addChild(mOpen);
	addChild(mRemove);
	refresh();
}

void TaskRow::refresh() {
	const Task* task = mBoard.task(mId);
	if (!task) return;

	std::string text = taskLabel(mId, *task) + "    " + std::string(statusLabel(task->status));
	if (task->session.state != SessionState::None)
		text += "  \u00b7  " + std::string(sessionStateLabel(task->session.state));
	mOpen.setText(text);
	mOpen.setSwatch(taskAccentColor(*task));
}

RelationSection::RelationSection(std::string title, std::string addText) :
	Collapsible(std::move(title)),
	mAdd(std::move(addText)) {
	setDebugName("relation section: " + header().title());

	mAdd.setStyle(Button::Style::Flat);
	setHeaderAction(&mAdd);
	mPicker.setPlaceholder("Find a task by #id, title or description");
	mPicker.setVisible(false);
	mList.setSizing(AxisSizing::fill(), AxisSizing::fit());
	setExpanded(true, false);

	body().addChild(mPicker);
	body().addChild(mList);
}

void RelationSection::setCount(size_t count) { header().setSummary(std::to_string(count)); }

void RelationSection::showPicker(bool shown) {
	mPicker.setVisible(shown);
	if (!shown) {
		mPicker.input().setText("");
		mPicker.hideResults();
		return;
	}
	setExpanded(true, false);
	mPicker.focus();
}

TaskDetails::TaskDetails(TaskBoard& board) :
	mBoard(board) {
	setDebugName("task details");
	buildLayout();
	bindEditors();
}

void TaskDetails::buildLayout() {
	columnLayout(*this, kSectionGap);

	for (Widget* widget : std::initializer_list<Widget*>{ &children, &dependsOn, &unblocks })
		addChild(*widget);
}

void TaskDetails::bindEditors() {
	children.list().setOnMoved([this](size_t from, size_t to) { childrenReordered(from, to); });
	dependsOn.list().setDragMode(ReorderableList::DragMode::None);
	unblocks.list().setDragMode(ReorderableList::DragMode::None);
	bindPicker(children, Relation::Child);
	bindPicker(dependsOn, Relation::DependsOn);
	bindPicker(unblocks, Relation::Unblocks);
}

void TaskDetails::bindPicker(RelationSection& section, Relation relation) {
	section.addButton().setOnActivate([this, &section] { section.showPicker(!section.isPickerShown()); });
	section.picker().setQuery([this, relation](const std::string& query) { return pickerResults(query, relation); });
	section.picker().setOnChosen([this, &section, relation](size_t index) {
		const std::string query = section.picker().input().text();
		const std::optional<NodeId> existing =
			index > 0 && index - 1 < mPickerChoices.size() ? std::optional(mPickerChoices[index - 1]) : std::nullopt;
		section.showPicker(false);
		picked(existing, relation, query);
	});
}

bool TaskDetails::canLink(NodeId other, Relation relation) const {
	switch (relation) {
		case Relation::Child: return !mShown || mBoard.canSetParent(other, *mShown);
		case Relation::DependsOn: return mShown && mBoard.canAddDependency(*mShown, other);
		case Relation::Unblocks: return mShown && mBoard.canAddDependency(other, *mShown);
	}
	return false;
}

bool TaskDetails::isLinked(NodeId other, Relation relation) const {
	std::vector<NodeId> existing;
	switch (relation) {
		case Relation::Child: existing = shownChildren(); break;
		case Relation::DependsOn: existing = mShown ? mBoard.dependenciesOf(*mShown) : existing; break;
		case Relation::Unblocks: existing = mShown ? mBoard.dependentsOf(*mShown) : existing; break;
	}
	return std::find(existing.begin(), existing.end(), other) != existing.end();
}

std::vector<SearchResult> TaskDetails::pickerResults(const std::string& query, Relation relation) {
	mPickerChoices.clear();
	std::vector<SearchResult> results;
	const std::string title = query.empty() ? std::string("New task") : query;
	results.push_back({ "+ Create new: " + title, "", true });

	const std::string needle = lowercaseText(query);
	const std::optional<NodeId> wantedId = taskIdQuery(query);
	for (const Node& node : mBoard.graph().nodes()) {
		const Task* task = mBoard.task(node.id);
		if (!task || node.id == mShown) continue;
		if (!query.empty()) {
			const bool matches =
				wantedId ? node.id == *wantedId : lowercaseText(task->title).find(needle) != std::string::npos;
			if (!matches) continue;
		}

		const bool linked = isLinked(node.id, relation);
		const bool allowed = canLink(node.id, relation);
		std::string detail =
			linked ? "already linked" : (allowed ? std::string(statusLabel(task->status)) : "would create a cycle");
		mPickerChoices.push_back(node.id);
		results.push_back({ taskLabel(node.id, *task), std::move(detail), allowed && !linked });
	}
	return results;
}

void TaskDetails::picked(std::optional<NodeId> existing, Relation relation, const std::string& query) {
	if (!existing) {
		const NodeId created = createTask(relation == Relation::Child ? mShown : mBoard.parentOf(mShown.value_or(0)));
		if (!query.empty()) mBoard.task(created)->title = query;
		if (relation != Relation::Child) link(created, relation);
		refreshRelations();
		if (mOnChanged) mOnChanged();
		if (mOnTaskCreated) mOnTaskCreated(created);
		return;
	}

	link(*existing, relation);
	refreshRelations();
	if (mOnChanged) mOnChanged();
}

void TaskDetails::link(NodeId other, Relation relation) {
	switch (relation) {
		case Relation::Child:
			if (!mShown) {
				mBoard.clearParent(other);
				break;
			}
			mBoard.setParent(other, *mShown);
			mBoard.placeLastAmongSiblings(other);
			break;
		case Relation::DependsOn: mBoard.addDependency(*mShown, other); break;
		case Relation::Unblocks: mBoard.addDependency(other, *mShown); break;
	}
}

void TaskDetails::removeLink(NodeId other, bool dependency) {
	if (!mShown) return;

	if (dependency) mBoard.removeDependency(*mShown, other);
	else mBoard.removeDependency(other, *mShown);
	refreshRelations();
}

std::vector<NodeId> TaskDetails::shownChildren() const {
	return mShown ? mBoard.childrenOf(*mShown) : mBoard.rootTasks();
}

void TaskDetails::childrenReordered(size_t from, size_t to) {
	const std::vector<NodeId> siblings = shownChildren();
	if (from >= siblings.size() || to >= siblings.size() || from == to) return;

	if (to < from) mBoard.placeBefore(siblings[from], siblings[to]);
	else mBoard.placeAfter(siblings[from], siblings[to]);
	refreshRelations();
	if (mOnChanged) mOnChanged();
}

NodeId TaskDetails::createTask(std::optional<NodeId> parent) {
	Task task;
	if (const Task* source = mShown ? mBoard.task(*mShown) : nullptr) {
		task.agentModel = source->agentModel;
		task.harness = source->harness;
		task.effort = source->effort;
		task.permissionMode = source->permissionMode;
	}
	task.order = mBoard.orderAfterLast();

	const Vec2F position =
		mShown ? mBoard.graph().findNode(*mShown)->position + Vec2F(0.0f, 160.0f) : Vec2F(40.0f, 40.0f);
	const NodeId id = mBoard.addTask(std::move(task), position);
	if (parent) mBoard.setParent(id, *parent);
	return id;
}

void TaskDetails::addChildTask() {
	const NodeId id = createTask(mShown);
	refreshRelations();
	if (mOnTaskCreated) mOnTaskCreated(id);
}

void TaskDetails::showTask(std::optional<NodeId> id) {
	if (id != mShown) {
		for (RelationSection* section : { &children, &dependsOn, &unblocks })
			section->showPicker(false);
	}
	mShown = id;

	const Task* task = id ? mBoard.task(*id) : nullptr;
	for (Widget* widget : std::initializer_list<Widget*>{ &dependsOn, &unblocks })
		widget->setVisible(task != nullptr);
	refreshRelations();
}

void TaskDetails::refreshRelations() {
	fillSection(children, mChildRows, shownChildren(), false);
	if (!mShown) return;

	fillSection(dependsOn, mDependencyRows, mBoard.dependenciesOf(*mShown), true);
	fillSection(unblocks, mDependentRows, mBoard.dependentsOf(*mShown), true);
}

void TaskDetails::fillSection(
	RelationSection& section, std::vector<std::unique_ptr<TaskRow>>& rows, const std::vector<NodeId>& ids, bool removable
) {
	section.list().clearItems();
	rows.clear();
	section.setCount(ids.size());

	const bool dependency = &section == &dependsOn;
	for (NodeId id : ids) {
		rows.push_back(std::make_unique<TaskRow>(mBoard, id, removable));
		TaskRow& taskRow = *rows.back();
		taskRow.setSizing(AxisSizing::fill(), AxisSizing::fit());
		taskRow.setOnPreviewed([this](NodeId previewed) {
			if (mOnPreviewed) mOnPreviewed(previewed);
		});
		taskRow.setOnOpened([this](NodeId opened) {
			if (mOnOpened) mOnOpened(opened);
		});
		taskRow.setOnRemoved([this, dependency](NodeId other) { removeLink(other, dependency); });
		section.list().addItem(taskRow);
	}
}

TaskRow* TaskDetails::row(const RelationSection& section, NodeId id) const {
	for (const auto& taskRow : rowsOf(section))
		if (taskRow->taskId() == id) return taskRow.get();
	return nullptr;
}

const std::vector<std::unique_ptr<TaskRow>>& TaskDetails::rowsOf(const RelationSection& section) const {
	if (&section == &children) return mChildRows;
	if (&section == &dependsOn) return mDependencyRows;
	return mDependentRows;
}

TaskPage::TaskPage(TaskBoard& board) :
	mDetails(board) {
	setDebugName("task page");
	columnLayout(*this, kSectionGap);
	rootHint.setTone(Label::Tone::Muted);
	addChild(rootHint);
	addChild(mDetails);
}

void TaskPage::showTask(std::optional<NodeId> id) {
	mShown = id;
	rootHint.setVisible(!id.has_value());
	mDetails.showTask(id);
}

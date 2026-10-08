#include "SessionPanel.hpp"

#include "BoxLayout.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kGap = 6;
	constexpr halnf kSectionGap = 14;
	constexpr halnf kSettingLabelWidth = 64;
	constexpr const char* kSummarySeparator = "  \u00b7  ";

	std::vector<std::string> optionIds(const std::vector<AgentOption>& options) {
		std::vector<std::string> ids;
		for (const AgentOption& option : options)
			ids.push_back(option.id);
		return ids;
	}

	std::vector<std::string> optionLabels(const std::vector<AgentOption>& options) {
		std::vector<std::string> labels;
		for (const AgentOption& option : options)
			labels.push_back(option.label);
		return labels;
	}

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

	void settingLabel(Label& label) {
		label.setTone(Label::Tone::Muted);
		label.setSizing(AxisSizing::fixed(kSettingLabelWidth), AxisSizing::fit());
	}
}

SubrouterStatus::SubrouterStatus() {
	setDebugName("subrouter status");
	columnLayout(*this, kGap);
	for (Label* label : { &hint, &accounts }) {
		label->setWrap(true);
		label->setSizing(AxisSizing::fill(), AxisSizing::fit());
		addChild(*label);
	}
	hint.setTone(Label::Tone::Muted);

	SubrouterInfo empty;
	empty.loginHint = "Run 'subrouter login anthropic' to add an account.";
	setInfo(empty);
}

void SubrouterStatus::setInfo(const SubrouterInfo& info) {
	if (!info.available) {
		hint.setText("Subrouter is not available on this machine.");
		hint.setVisible(true);
		accounts.setVisible(false);
		return;
	}

	hint.setText(info.loginHint);
	hint.setVisible(!info.loginHint.empty() && info.accounts.empty());
	accounts.setVisible(!info.accounts.empty());
	if (info.accounts.empty()) return;

	std::string text = "Subscription accounts:";
	for (const SubrouterAccount& account : info.accounts) {
		text += std::string("\n") + (account.coolingDown ? "[!]  " : "[ok]  ") + account.email;
		if (!account.coolingDown) continue;
		text += "  rate limited";
		if (!account.cooldownUntil.empty()) text += " until " + account.cooldownUntil;
	}
	accounts.setText(text);
}

SessionPresetPicker::SessionPresetPicker(SessionPresets& presets) :
	mPresets(presets) {
	setDebugName("session preset picker");
	buildLayout();
	bindControls();
	refreshOptions();
}

void SessionPresetPicker::buildLayout() {
	settingLabel(mLabel);
	preset.setSizing(AxisSizing::fill(), AxisSizing::fit());
	remove.setStyle(Button::Style::Flat);
	remove.setTooltip("Delete this preset");
	rowLayout(mPresetRow, kGap);
	for (Widget* widget : std::initializer_list<Widget*>{ &mLabel, &preset, &saveAs, &remove })
		mPresetRow.addChild(*widget);

	name.setPlaceholder("Preset name, e.g. smart or dumb agent");
	name.setSizing(AxisSizing::fill(), AxisSizing::fit());
	save.setStyle(Button::Style::Accent);
	rowLayout(mNamingRow, kGap);
	for (Widget* widget : std::initializer_list<Widget*>{ &name, &save, &cancel })
		mNamingRow.addChild(*widget);
	mNamingRow.setVisible(false);

	problem.setTone(Label::Tone::Warning);
	problem.setWrap(true);
	problem.setSizing(AxisSizing::fill(), AxisSizing::fit());
	problem.setVisible(false);

	columnLayout(*this, kGap);
	addChild(mPresetRow);
	addChild(mNamingRow);
	addChild(problem);
}

void SessionPresetPicker::bindControls() {
	preset.setOnSelected([this](long option) { presetChosen(option); });
	saveAs.setOnActivate([this] { openNaming(); });
	remove.setOnActivate([this] { removeSelected(); });
	save.setOnActivate([this] { commitName(); });
	cancel.setOnActivate([this] { closeNaming(); });
	name.setOnCommitted([this](const std::string&) {
		if (name.isEditing()) commitName();
	});
}

void SessionPresetPicker::refreshOptions() {
	std::vector<std::string> options = { kCustomOption };
	for (const SessionPreset& entry : mPresets.all())
		options.push_back(entry.name);

	const bool wasLoading = mLoading;
	mLoading = true;
	preset.setOptions(std::move(options));
	preset.setSelectedIndex(mShownPreset ? long(*mShownPreset) + 1 : 0);
	mLoading = wasLoading;
	remove.setEnabled(mShownPreset.has_value());
}

void SessionPresetPicker::showSettings(const LaunchSettings& settings) {
	mShownPreset = mPresets.matching(settings);
	refreshOptions();
}

std::optional<size_t> SessionPresetPicker::selectedPreset() const {
	const long option = preset.selectedIndex();
	if (option <= 0) return std::nullopt;
	return size_t(option - 1);
}

void SessionPresetPicker::presetChosen(long option) {
	if (mLoading) return;
	const std::optional<size_t> chosen = option > 0 ? std::optional(size_t(option - 1)) : std::nullopt;
	mShownPreset = chosen;
	remove.setEnabled(chosen.has_value());
	if (chosen && *chosen < mPresets.all().size() && mOnApplied) mOnApplied(mPresets.all()[*chosen].settings);
}

void SessionPresetPicker::openNaming() {
	const std::optional<size_t> shown = selectedPreset();
	name.setText(shown ? mPresets.all()[*shown].name : std::string());
	problem.setVisible(false);
	mNamingRow.setVisible(true);
}

void SessionPresetPicker::closeNaming() {
	mNamingRow.setVisible(false);
	name.setText("");
}

void SessionPresetPicker::commitName() {
	const std::string chosen = name.text();
	if (chosen.empty() || !mCurrent) return;

	mShownPreset = mPresets.save(chosen, mCurrent());
	closeNaming();
	refreshOptions();
	problem.setText("Couldn't write presets to " + mPresets.storagePath());
	problem.setVisible(mPresets.lastSaveFailed());
}

void SessionPresetPicker::removeSelected() {
	const std::optional<size_t> shown = selectedPreset();
	if (!shown) return;

	mPresets.remove(*shown);
	mShownPreset.reset();
	refreshOptions();
}

SessionTab::SessionTab(TaskBoard& board, SessionPresets& presetStore) :
	presets(presetStore),
	mBoard(board) {
	setDebugName("session tab");
	buildLayout();
	bindControls();
	setAgentOptions(mOptions);
}

void SessionTab::buildLayout() {
	state.setWrap(true);
	state.setSizing(AxisSizing::fill(), AxisSizing::fit());
	buildParameters();

	startSession.setStyle(Button::Style::Accent);
	rowLayout(mControls, kGap);
	for (Widget* widget : std::initializer_list<Widget*>{ &startSession, &stopSession, &openSession })
		mControls.addChild(*widget);

	restartHint.setTone(Label::Tone::Warning);
	restartHint.setWrap(true);
	restartHint.setSizing(AxisSizing::fill(), AxisSizing::fit());
	rowLayout(mRestartRow, kGap);
	mRestartRow.addChild(restartKeep);
	mRestartRow.addChild(restartFresh);

	columnLayout(*this, kSectionGap);
	for (Widget* widget : std::initializer_list<Widget*>{
				 &state, &presets, &parameters, &mControls, &restartHint, &mRestartRow, &subrouter })
		addChild(*widget);
}

void SessionTab::buildParameters() {
	parameters.setSizing(AxisSizing::fill(), AxisSizing::fit());
	parameters.body().emplaceLayout<ColumnLayout>(kGap);
	parameters.setExpanded(false, false);

	const std::pair<const char*, Dropdown*> settings[] = {
		{ "Model", &model }, { "Harness", &harness }, { "Effort", &effort }, { "Mode", &permissionMode }
	};
	for (const auto& [text, dropdown] : settings) {
		mSettingRows.push_back(std::make_unique<Widget>());
		Widget& row = *mSettingRows.back();
		rowLayout(row, kGap);
		mSettingLabels.push_back(std::make_unique<Label>(text));
		settingLabel(*mSettingLabels.back());
		dropdown->setSizing(AxisSizing::fill(), AxisSizing::fit());
		row.addChild(*mSettingLabels.back());
		row.addChild(*dropdown);
		parameters.body().addChild(row);
	}
}

void SessionTab::bindControls() {
	model.setOnSelected([this](long index) {
		change([&](Task& task) { task.agentModel = mOptions.models[size_t(index)].id; });
	});
	harness.setOnSelected([this](long index) {
		change([&](Task& task) { task.harness = mOptions.harnesses[size_t(index)].id; });
	});
	effort.setOnSelected([this](long index) {
		change([&](Task& task) { task.effort = mOptions.efforts[size_t(index)]; });
	});
	permissionMode.setOnSelected([this](long index) {
		change([&](Task& task) { task.permissionMode = mOptions.permissionModes[size_t(index)]; });
	});
	presets.setOnApplied([this](const LaunchSettings& settings) { applyPreset(settings); });
	presets.setCurrentSettings([this] {
		const Task* task = mShown ? mBoard.task(*mShown) : nullptr;
		return task ? task->launchSettings() : LaunchSettings{};
	});

	startSession.setOnActivate([this] {
		if (mShown && mStore) mStore->startSession(*mShown);
	});
	stopSession.setOnActivate([this] {
		if (mShown && mStore) mStore->stopSession(*mShown);
	});
	restartKeep.setOnActivate([this] {
		if (mShown && mStore) mStore->restartSession(*mShown, false);
	});
	restartFresh.setOnActivate([this] {
		if (mShown && mStore) mStore->restartSession(*mShown, true);
	});
	openSession.setOnActivate([this] {
		if (mShown && mOnOpenSession) mOnOpenSession(*mShown);
	});
}

void SessionTab::setAgentOptions(AgentOptions options) {
	mOptions = std::move(options);
	model.setOptions(optionLabels(mOptions.models));
	harness.setOptions(optionLabels(mOptions.harnesses));
	effort.setOptions(mOptions.efforts);
	permissionMode.setOptions(mOptions.permissionModes);
	showTask(mShown);
}

void SessionTab::change(const std::function<void(Task&)>& edit) {
	Task* task = mShown ? mBoard.task(*mShown) : nullptr;
	if (!task || mLoading) return;

	edit(*task);
	presets.showSettings(task->launchSettings());
	parameters.header().setSummary(parametersSummary(*task));
	refreshRestart(*task);
	if (mOnChanged) mOnChanged();
}

void SessionTab::applyPreset(const LaunchSettings& settings) {
	Task* task = mShown ? mBoard.task(*mShown) : nullptr;
	if (!task || mLoading) return;

	task->agentModel = settings.agentModel;
	task->harness = settings.harness;
	task->effort = settings.effort;
	task->permissionMode = settings.permissionMode;
	showSettings(*task);
	refreshRestart(*task);
	if (mOnChanged) mOnChanged();
}

void SessionTab::showTask(std::optional<NodeId> id) {
	mShown = id;
	const Task* task = id ? mBoard.task(*id) : nullptr;
	if (!task) return;

	showSettings(*task);
	refreshControls(*task);
}

void SessionTab::showSettings(const Task& task) {
	const bool wasLoading = mLoading;
	mLoading = true;
	model.setSelectedIndex(optionIndex(optionIds(mOptions.models), task.agentModel));
	harness.setSelectedIndex(optionIndex(optionIds(mOptions.harnesses), task.harness));
	effort.setSelectedIndex(optionIndex(mOptions.efforts, task.effort));
	permissionMode.setSelectedIndex(optionIndex(mOptions.permissionModes, task.permissionMode));
	mLoading = wasLoading;
	presets.showSettings(task.launchSettings());
	parameters.header().setSummary(parametersSummary(task));
}

std::string SessionTab::parametersSummary(const Task& task) const {
	const LaunchSettings settings = task.launchSettings();
	return settings.agentModel + kSummarySeparator + settings.harness + kSummarySeparator + settings.effort +
				 kSummarySeparator + settings.permissionMode;
}

void SessionTab::refreshControls(const Task& task) {
	const SessionState current = task.session.state;
	const bool running = task.session.isRunning();
	std::string text = std::string(sessionStateLabel(current));
	if (!task.session.detail.empty() && current != SessionState::None) text += kSummarySeparator + task.session.detail;
	state.setText(text);

	startSession.setText(current == SessionState::Ended ? "Resume session" : "Start session");
	startSession.setEnabled(!running);
	stopSession.setEnabled(running);
	openSession.setEnabled(running || task.harness == "dsh");
	refreshRestart(task);
}

void SessionTab::refreshRestart(const Task& task) {
	const bool running = task.session.isRunning();
	const bool needsRestart = running && task.sessionSettingsChanged();
	restartHint.setVisible(needsRestart);
	mRestartRow.setVisible(needsRestart);
	if (!needsRestart) return;

	const LaunchSettings& was = *task.session.launchedWith;
	restartHint.setText(
		"Running with " + was.agentModel + " on " + (was.harness.empty() ? "claude-code" : was.harness) + " / " +
		(was.effort.empty() ? "default" : was.effort) + " / " + was.permissionMode +
		". Save, then restart to use the new settings."
	);
}

long SessionTab::optionIndex(const std::vector<std::string>& values, const std::string& value) {
	auto found = std::find(values.begin(), values.end(), value);
	return found == values.end() ? -1 : long(found - values.begin());
}

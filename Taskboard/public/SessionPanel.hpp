#pragma once

#include "Button.hpp"
#include "Collapsible.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "SessionPresets.hpp"
#include "TaskBoard.hpp"
#include "TaskStore.hpp"
#include "TextField.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tp::taskboard {

	class SubrouterStatus : public ui::Widget {
	public:
		SubrouterStatus();
		void setInfo(const SubrouterInfo& info);

		ui::Label hint;
		ui::Label accounts;
	};

	class SessionPresetPicker : public ui::Widget {
	public:
		explicit SessionPresetPicker(SessionPresets& presets);

		void showSettings(const LaunchSettings& settings);
		void openNaming();
		[[nodiscard]] bool isNaming() const { return mNamingRow.isVisible(); }

		ui::Dropdown preset;
		ui::Button saveAs{ "Save as preset..." };
		ui::Button remove{ "\u2715" };
		ui::TextField name;
		ui::Button save{ "Save" };
		ui::Button cancel{ "Cancel" };
		ui::Label problem;

		void setOnApplied(std::function<void(const LaunchSettings&)> callback) { mOnApplied = std::move(callback); }
		void setCurrentSettings(std::function<LaunchSettings()> current) { mCurrent = std::move(current); }

		static constexpr const char* kCustomOption = "Custom";

	private:
		void buildLayout();
		void bindControls();
		void refreshOptions();
		void presetChosen(long option);
		void commitName();
		void closeNaming();
		void removeSelected();
		[[nodiscard]] std::optional<size_t> selectedPreset() const;

		SessionPresets& mPresets;
		std::optional<size_t> mShownPreset;
		bool mLoading = false;
		ui::Label mLabel{ "Preset" };
		ui::Widget mPresetRow;
		ui::Widget mNamingRow;
		std::function<void(const LaunchSettings&)> mOnApplied;
		std::function<LaunchSettings()> mCurrent;
	};

	class SessionTab : public ui::Widget {
	public:
		SessionTab(TaskBoard& board, SessionPresets& presetStore);
		void setSubrouterInfo(const SubrouterInfo& info) { subrouter.setInfo(info); }

		void showTask(std::optional<ui::NodeId> id);
		void setAgentOptions(AgentOptions options);
		void setStore(TaskStore* store) { mStore = store; }
		[[nodiscard]] bool isOfferingRestart() const { return mRestartRow.isVisible(); }

		ui::Label state;
		SessionPresetPicker presets;
		ui::Collapsible parameters{ "Parameters" };
		ui::Dropdown model;
		ui::Dropdown harness;
		ui::Dropdown effort;
		ui::Dropdown permissionMode;
		ui::Button startSession{ "Start session" };
		ui::Button stopSession{ "Stop" };
		ui::Button openSession{ "Open session" };
		ui::Label restartHint;
		ui::Button restartKeep{ "Restart, keep conversation" };
		ui::Button restartFresh{ "Restart fresh" };
		SubrouterStatus subrouter;

		void setOnChanged(std::function<void()> callback) { mOnChanged = std::move(callback); }
		void setOnOpenSession(std::function<void(ui::NodeId)> callback) { mOnOpenSession = std::move(callback); }

	private:
		void buildLayout();
		void buildParameters();
		void bindControls();
		void change(const std::function<void(Task&)>& edit);
		void applyPreset(const LaunchSettings& settings);
		void showSettings(const Task& task);
		void refreshControls(const Task& task);
		void refreshRestart(const Task& task);
		[[nodiscard]] std::string parametersSummary(const Task& task) const;
		[[nodiscard]] static long optionIndex(const std::vector<std::string>& values, const std::string& value);

		TaskBoard& mBoard;
		TaskStore* mStore = nullptr;
		std::optional<ui::NodeId> mShown;
		bool mLoading = false;
		AgentOptions mOptions = offlineAgentOptions();
		ui::Widget mControls;
		std::vector<std::unique_ptr<ui::Widget>> mSettingRows;
		std::vector<std::unique_ptr<ui::Label>> mSettingLabels;
		ui::Widget mRestartRow;
		std::function<void()> mOnChanged;
		std::function<void(ui::NodeId)> mOnOpenSession;
	};
}

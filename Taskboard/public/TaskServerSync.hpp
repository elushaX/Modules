#pragma once

#include "BoardSnapshot.hpp"
#include "McpClient.hpp"
#include "TaskBoard.hpp"
#include "TaskStore.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace tp::taskboard {

	std::string colorToHex(const RGBA& color);
	std::optional<RGBA> colorFromHex(const std::string& text);
	const char* serverFieldName(TaskField field);
	picojson::value fieldValue(const Task& task, TaskField field);
	void setFieldValue(Task& task, TaskField field, const picojson::value& value);
	std::string fieldLabel(TaskField field);

	struct FieldConflict {
		ui::NodeId task = 0;
		TaskField field = TaskField::Title;
		picojson::value mine;
		picojson::value theirs;
	};

	struct HistorySpan {
		std::optional<double> first;
		std::optional<double> last;
	};

	// Keeps the board in step with the task server.
	// `mSaved` is the board as the server last had it. The user's unsaved work is the difference between the
	// live board and `mSaved`; reloads apply server changes only where the user has no unsaved change.
	class TaskServerSync : public TaskStore, public SecretsStore {
	public:
		TaskServerSync(TaskBoard& board, McpClient& client);

		bool load();
		void processResponses();
		void pollForChanges();

		bool viewHistoryAt(std::optional<double> moment);
		[[nodiscard]] std::optional<double> viewedMoment() const { return mViewedMoment; }
		[[nodiscard]] bool isViewingHistory() const { return mViewedMoment.has_value(); }
		[[nodiscard]] const HistorySpan& historySpan() const { return mHistorySpan; }
		[[nodiscard]] const std::vector<double>& historyChanges() const { return mHistoryChanges; }

		void save();
		[[nodiscard]] BoardChanges unsavedChanges() const;
		[[nodiscard]] bool hasUnsavedChanges() const { return !unsavedChanges().empty(); }
		[[nodiscard]] bool isSaving() const { return mSaving; }

		void startSession(ui::NodeId id) override;
		void restartSession(ui::NodeId id, bool fresh) override;
		void stopSession(ui::NodeId id) override;
		void beginEditing(ui::NodeId id, TaskField field) override;
		void endEditing() override;

		void listSecrets(std::function<void(std::vector<SecretEntry>)> onResult) override;
		void setSecret(
			const std::string& key, const std::string& value, const std::string& description,
			std::function<void(bool ok, std::string error)> onDone
		) override;
		void deleteSecret(const std::string& key, std::function<void(bool ok, std::string error)> onDone) override;

		bool loadOptions();
		[[nodiscard]] const AgentOptions& options() const { return mOptions; }

		void resolveConflict(FieldConflict conflict, bool keepMine);
		[[nodiscard]] const std::vector<FieldConflict>& conflicts() const { return mConflicts; }
		[[nodiscard]] const std::string& lastError() const { return mLastError; }

		void loadSubrouterInfo();

		void setOnBoardReloaded(std::function<void()> callback) { mOnBoardReloaded = std::move(callback); }
		void setOnHistoryChanged(std::function<void()> callback) { mOnHistoryChanged = std::move(callback); }
		void setOnSaved(std::function<void()> callback) { mOnSaved = std::move(callback); }
		void setOnError(std::function<void(const std::string&)> callback) { mOnError = std::move(callback); }
		void setOnConflictsChanged(std::function<void()> callback) { mOnConflictsChanged = std::move(callback); }
		void setOnSubrouterInfo(std::function<void(const SubrouterInfo&)> callback) {
			mOnSubrouterInfo = std::move(callback);
		}

	private:
		void applyBoard(const picojson::object& board);
		void applyPalette(const picojson::object& board, const BoardSnapshot& local);
		std::vector<TaskLink> applyTasks(const picojson::object& board, const BoardSnapshot& local);
		void
		applyLinks(const picojson::object& board, const std::vector<TaskLink>& parentLinks, const BoardSnapshot& local);
		void mergeTask(ui::NodeId id, const Task& incoming, const NodeLayout& layout, const BoardSnapshot& local);
		void connect(const TaskLink& link);
		void disconnect(const TaskLink& link);

		[[nodiscard]] picojson::object saveRequest(const BoardChanges& changes) const;
		void finishSave(const McpToolResult& result, const BoardChanges& changes);
		void adoptCreatedIds(const picojson::object& ids);

		void addConflict(FieldConflict conflict);
		bool dropConflict(ui::NodeId id, TaskField field);
		void requestReload();
		void requestBoard(std::optional<double> moment);
		void boardArrived(const McpToolResult& result, unsigned generation);
		void applyHistorySpan(const picojson::object& board);
		void loadHistoryChanges();
		void sessionCall(const std::string& tool, ui::NodeId id, picojson::object arguments);
		void reportError(const std::string& error);
		[[nodiscard]] bool isEditing(ui::NodeId id, TaskField field) const;

		TaskBoard& mBoard;
		McpClient& mClient;
		BoardSnapshot mSaved;
		std::string mRevision;
		std::string mLastError;
		std::vector<FieldConflict> mConflicts;
		AgentOptions mOptions = offlineAgentOptions();
		std::optional<std::pair<ui::NodeId, TaskField>> mEditing;
		std::optional<double> mViewedMoment;
		std::optional<std::optional<double>> mQueuedMoment;
		HistorySpan mHistorySpan;
		std::vector<double> mHistoryChanges;
		unsigned mBoardGeneration = 0;
		bool mHistoryInFlight = false;
		bool mPollInFlight = false;
		bool mSaving = false;
		bool mSubrouterInFlight = false;
		int mSubrouterPollTick = 0;
		std::function<void()> mOnBoardReloaded;
		std::function<void()> mOnSaved;
		std::function<void()> mOnHistoryChanged;
		std::function<void(const std::string&)> mOnError;
		std::function<void()> mOnConflictsChanged;
		std::function<void(const SubrouterInfo&)> mOnSubrouterInfo;
	};
}

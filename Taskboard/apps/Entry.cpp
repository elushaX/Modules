#include "DemoBoard.hpp"
#include "McpClient.hpp"
#include "TaskTreeView.hpp"
#include "TaskServerSync.hpp"
#include "WidgetApplication.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <mutex>
#include <string>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace tp;
using namespace tp::taskboard;

namespace {
	constexpr std::chrono::milliseconds kPollInterval{ 1000 };

	class PeriodicWake {
	public:
		explicit PeriodicWake(std::function<void()> tick) :
			mThread([this, tick = std::move(tick)] { run(tick); }) {}

		~PeriodicWake() {
			{
				std::lock_guard lock(mMutex);
				mStopping = true;
			}
			mStopped.notify_all();
			mThread.join();
		}

		[[nodiscard]] bool takeDue() { return mDue.exchange(false); }

	private:
		void run(const std::function<void()>& tick) {
			std::unique_lock lock(mMutex);
			while (!mStopped.wait_for(lock, kPollInterval, [this] { return mStopping; })) {
				mDue = true;
				tick();
			}
		}

		std::mutex mMutex;
		std::condition_variable mStopped;
		bool mStopping = false;
		std::atomic<bool> mDue = false;
		std::thread mThread;
	};

	const char* environmentOr(const char* name, const char* fallback) {
		const char* value = std::getenv(name);
		return value && *value ? value : fallback;
	}

	McpServerCommand taskServerCommand() {
		const std::string directory = environmentOr("TASKBOARD_SERVER_DIR", TASKBOARD_SERVER_DIR);

		McpServerCommand command;
		command.executable = "uv";
		command.arguments = { "run", "--project", directory, "python", directory + "/server.py" };
		command.stderrPath = (std::filesystem::temp_directory_path() / "taskboard-server.log").string();
		for (const char* name : { "TASKS_DB", "AGENTS_DATA_DIR", "AGENTS_SESSIONS_DIR", "AGENTS_TMUX_SOCKET" }) {
			if (const char* value = std::getenv(name)) command.environment.emplace_back(name, value);
		}
		return command;
	}

	bool launchDetached(const std::vector<std::string>& command) {
		const pid_t child = ::fork();
		if (child < 0) return false;

		if (child == 0) {
			::setsid();
			if (::fork() != 0) ::_exit(0);

			std::vector<char*> argv;
			for (const std::string& part : command)
				argv.push_back(const_cast<char*>(part.c_str()));
			argv.push_back(nullptr);
			::execvp(argv[0], argv.data());
			::_exit(127);
		}

		int status = 0;
		::waitpid(child, &status, 0);
		return WIFEXITED(status) && WEXITSTATUS(status) == 0;
	}

	bool demoRequested(int argc, char** argv) {
		for (int index = 1; index < argc; index++) {
			if (std::strcmp(argv[index], "--demo") == 0) return true;
		}
		return false;
	}
}

class TaskBoardApplication : public ui::WidgetApplication {
public:
	explicit TaskBoardApplication(bool demo) :
		WidgetApplication({ MODULES_APP_ID, MODULES_APP_NAME }) {
		mView.attachMenus(menuBar(), [this] { ui().windowHost()->closeWindow(); });
		setRoot(mView);

		if (demo) startDemo();
		else connectToServer();
	}

	[[nodiscard]] AppStorage persistentAppStorage() const override { return taskboardStorage(); }

	~TaskBoardApplication() override {
		mPollTimer.reset();
		mClient.setOnResponse({});
		mView.setStore(nullptr);
		mSync.reset();
		mClient.stop();
	}

private:
	void startDemo() {
		populateDemoBoard(mBoard);
		mView.status.setText("Demo board (not saved)");
		mView.setOnOpenSession([this](ui::NodeId id) {
			mView.status.setText("Session for task #" + std::to_string(id) + " is not connected yet");
		});
	}

	void connectToServer() {
		mClient.setOnResponse([this] { window().wake(); });

		std::string error;
		if (!mClient.start(taskServerCommand(), error)) {
			mView.status.setText("Task server unavailable: " + error);
			return;
		}

		mSync.emplace(mBoard, mClient);
		mSync->setOnError([this](const std::string& message) { mView.status.setText("Task server: " + message); });
		if (!mSync->load()) return;

		mSync->setOnConflictsChanged([this] { showConflicts(); });
		mView.header().setOnConflictChoice([this](bool keepMine) { resolveShownConflicts(keepMine); });
		if (mSync->loadOptions()) mView.setAgentOptions(mSync->options());
		mSync->setOnBoardReloaded([this] {
			mView.boardReloaded();
			refreshSaveState();
		});
		mSync->setOnSaved([this] { refreshSaveState(); });
		mSync->setOnHistoryChanged([this] { showHistoryState(); });
		mView.timeline().setOnMomentChosen([this](std::optional<double> moment) { mSync->viewHistoryAt(moment); });
		showHistoryState();
		mView.setOnBoardEdited([this] { refreshSaveState(); });
		mView.setOnSave([this] {
			mSync->save();
			refreshSaveState();
		});
		mSync->setOnSubrouterInfo([this](const SubrouterInfo& info) { mView.session().setSubrouterInfo(info); });
		mSync->loadSubrouterInfo();
		mView.setStore(&*mSync);
		mView.setSecretsStore(&*mSync);
		refreshSaveState();
		mView.setOnOpenSession([this](ui::NodeId id) { openSession(id); });
		mPollTimer.emplace([this] { window().wake(); });
	}

	void processFrame(EventHandler* eventHandler, halnf deltaTime) override {
		if (mSync) {
			mSync->processResponses();
			if (mPollTimer && mPollTimer->takeDue()) mSync->pollForChanges();
		}
		WidgetApplication::processFrame(eventHandler, deltaTime);
	}

	void refreshSaveState() {
		if (mSync) mView.showSaveState(mSync->unsavedChanges().count(), mSync->isSaving());
	}

	void showHistoryState() {
		const HistorySpan& span = mSync->historySpan();
		mView.timeline().setSpan(span.first, span.last);
		mView.timeline().setChangeMoments(mSync->historyChanges());
		mView.timeline().showMoment(mSync->viewedMoment());
		mView.setReadOnly(mSync->isViewingHistory());
	}

	void showConflicts() {
		std::vector<std::string> summaries;
		for (const FieldConflict& conflict : mSync->conflicts())
			summaries.push_back(
				"#" + std::to_string(conflict.task) + " " + fieldLabel(conflict.field) + ": theirs \"" +
				conflict.theirs.to_str() + "\", yours \"" + conflict.mine.to_str() + "\""
			);
		mView.showConflicts(summaries);
	}

	void resolveShownConflicts(bool keepMine) {
		const std::vector<FieldConflict> pending = mSync->conflicts();
		for (const FieldConflict& conflict : pending)
			mSync->resolveConflict(conflict, keepMine);
	}

	void openSession(ui::NodeId id) {
		const Task* task = mBoard.task(id);
		if (task && task->harness == "dsh") {
			openDshChat(id, *task);
			return;
		}
		attachTerminal(id, "task-" + std::to_string(id));
	}

	void openDshChat(ui::NodeId id, const Task& task) {
		if (!task.session.isRunning()) mSync->startSession(id);

		const std::string url = "dsh-app://task/" + std::to_string(id);
		if (!ui().windowHost()->openUrl(url)) {
			mView.status.setText("Could not open " + url + "; install Taskboard/dsh-deeplink (see its README)");
			return;
		}
		mView.status.setText("dsh session for task #" + std::to_string(id) + " opened in the DSH app window");
	}

	void attachTerminal(ui::NodeId id, const std::string& tmuxTarget) {
		const std::string terminal = environmentOr("TASKBOARD_TERMINAL", "kitty");
		const std::string title = "task #" + std::to_string(id);

		if (!launchDetached({ terminal, "--title", title, "tmux", "attach", "-t", tmuxTarget })) {
			mView.status.setText("Could not start " + terminal + " for task #" + std::to_string(id));
			return;
		}
		mView.status.setText("Session for task #" + std::to_string(id) + " is running in tmux " + tmuxTarget);
	}

	TaskBoard mBoard;
	TaskTreeView mView{ mBoard };
	McpClient mClient;
	std::optional<TaskServerSync> mSync;
	std::optional<PeriodicWake> mPollTimer;
};

int main(int argc, char** argv) {
	TaskBoardApplication application(demoRequested(argc, argv));
	application.run();
	return 0;
}

#pragma once

#include "picojson.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tp::taskboard {

	struct McpServerCommand {
		std::string executable;
		std::vector<std::string> arguments;
		std::vector<std::pair<std::string, std::string>> environment;
		std::string stderrPath;
	};

	struct McpToolResult {
		bool ok = false;
		picojson::value value;
		std::string error;
	};

	class McpClient {
	public:
		using Completion = std::function<void(const McpToolResult&)>;

		McpClient();
		~McpClient();

		McpClient(const McpClient&) = delete;
		McpClient& operator=(const McpClient&) = delete;

		bool start(const McpServerCommand& command, std::string& error);
		void stop();
		[[nodiscard]] bool isRunning() const;

		void callTool(const std::string& name, picojson::object arguments, Completion completion);
		McpToolResult callToolBlocking(const std::string& name, picojson::object arguments);

		void dispatchCompletions();
		[[nodiscard]] size_t pendingCount() const;
		void waitForIdle();
		void setOnResponse(std::function<void()> callback);

	private:
		struct Pending {
			Completion completion;
		};

		bool spawn(const McpServerCommand& command, std::string& error);
		bool handshake(std::string& error);
		void readLoop();
		void handleMessage(const std::string& line);
		long nextId();
		bool send(const picojson::object& message);
		bool request(const std::string& method, picojson::object params, Completion completion);
		bool waitUntil(const std::function<bool()>& finished);
		void failAll(const std::string& reason);
		void deliver(Completion completion, McpToolResult result);

		[[nodiscard]] static McpToolResult toolResult(const picojson::value& response);

		int mPid = -1;
		int mToServer = -1;
		int mFromServer = -1;
		std::thread mReader;

		mutable std::mutex mMutex;
		std::condition_variable mResponded;
		std::map<long, Pending> mPending;
		std::deque<std::pair<Completion, McpToolResult>> mReady;
		long mNextId = 1;
		bool mRunning = false;
		std::function<void()> mOnResponse;
	};
}

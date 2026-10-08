#include "McpClient.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace tp;
using namespace tp::taskboard;

namespace {
	constexpr const char* kProtocolVersion = "2024-11-05";

	std::string errorText(const std::string& context) { return context + ": " + std::strerror(errno); }

	bool writeAll(int descriptor, const std::string& data) {
		size_t written = 0;
		while (written < data.size()) {
			const ssize_t count = ::write(descriptor, data.data() + written, data.size() - written);
			if (count < 0 && errno == EINTR) continue;
			if (count <= 0) return false;
			written += size_t(count);
		}
		return true;
	}

	picojson::object clientInfo() {
		picojson::object info;
		info["name"] = picojson::value("taskboard");
		info["version"] = picojson::value("1");
		return info;
	}
}

McpClient::McpClient() = default;

McpClient::~McpClient() { stop(); }

bool McpClient::start(const McpServerCommand& command, std::string& error) {
	if (isRunning()) return true;
	if (!spawn(command, error)) return false;

	{
		std::lock_guard lock(mMutex);
		mRunning = true;
	}
	mReader = std::thread([this] { readLoop(); });

	if (handshake(error)) return true;
	stop();
	return false;
}

bool McpClient::spawn(const McpServerCommand& command, std::string& error) {
	int toChild[2];
	int fromChild[2];
	if (::pipe2(toChild, O_CLOEXEC) != 0 || ::pipe2(fromChild, O_CLOEXEC) != 0) {
		error = errorText("pipe");
		return false;
	}

	const pid_t pid = ::fork();
	if (pid < 0) {
		error = errorText("fork");
		return false;
	}

	if (pid == 0) {
		::dup2(toChild[0], STDIN_FILENO);
		::dup2(fromChild[1], STDOUT_FILENO);
		if (!command.stderrPath.empty()) {
			const int log = ::open(command.stderrPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (log >= 0) ::dup2(log, STDERR_FILENO);
		}
		for (const auto& [name, value] : command.environment)
			::setenv(name.c_str(), value.c_str(), 1);

		std::vector<char*> argv;
		argv.push_back(const_cast<char*>(command.executable.c_str()));
		for (const std::string& argument : command.arguments)
			argv.push_back(const_cast<char*>(argument.c_str()));
		argv.push_back(nullptr);
		::execvp(command.executable.c_str(), argv.data());
		::_exit(127);
	}

	::close(toChild[0]);
	::close(fromChild[1]);
	mPid = pid;
	mToServer = toChild[1];
	mFromServer = fromChild[0];
	return true;
}

bool McpClient::handshake(std::string& error) {
	picojson::object params;
	params["protocolVersion"] = picojson::value(kProtocolVersion);
	params["capabilities"] = picojson::value(picojson::object());
	params["clientInfo"] = picojson::value(clientInfo());

	bool done = false;
	McpToolResult outcome;
	const bool sent = request("initialize", params, [&](const McpToolResult& result) {
		outcome = result;
		done = true;
	});
	if (!sent || !waitUntil([&] { return done; }) || !outcome.ok) {
		error = outcome.error.empty() ? "task server did not answer initialize" : outcome.error;
		return false;
	}

	picojson::object notification;
	notification["jsonrpc"] = picojson::value("2.0");
	notification["method"] = picojson::value("notifications/initialized");
	return send(notification);
}

bool McpClient::waitUntil(const std::function<bool()>& finished) {
	while (true) {
		dispatchCompletions();
		if (finished()) return true;

		std::unique_lock lock(mMutex);
		mResponded.wait(lock, [&] { return !mReady.empty() || !mRunning; });
		if (mReady.empty() && !mRunning) {
			lock.unlock();
			dispatchCompletions();
			return finished();
		}
	}
}

void McpClient::stop() {
	{
		std::lock_guard lock(mMutex);
		if (mPid < 0 && !mReader.joinable()) return;
	}

	if (mToServer >= 0) {
		::close(mToServer);
		mToServer = -1;
	}

	if (mPid > 0) {
		int status = 0;
		::waitpid(mPid, &status, 0);
		mPid = -1;
	}

	if (mReader.joinable()) mReader.join();
	if (mFromServer >= 0) {
		::close(mFromServer);
		mFromServer = -1;
	}
	failAll("server stopped");
}

bool McpClient::isRunning() const {
	std::lock_guard lock(mMutex);
	return mRunning;
}

long McpClient::nextId() {
	std::lock_guard lock(mMutex);
	return mNextId++;
}

bool McpClient::send(const picojson::object& message) {
	if (mToServer < 0) return false;
	return writeAll(mToServer, picojson::value(message).serialize() + "\n");
}

bool McpClient::request(const std::string& method, picojson::object params, Completion completion) {
	const long id = nextId();
	{
		std::lock_guard lock(mMutex);
		if (!mRunning) return false;
		mPending[id] = { std::move(completion) };
	}

	picojson::object message;
	message["jsonrpc"] = picojson::value("2.0");
	message["id"] = picojson::value(double(id));
	message["method"] = picojson::value(method);
	message["params"] = picojson::value(params);
	if (send(message)) return true;

	std::lock_guard lock(mMutex);
	mPending.erase(id);
	return false;
}

void McpClient::callTool(const std::string& name, picojson::object arguments, Completion completion) {
	picojson::object params;
	params["name"] = picojson::value(name);
	params["arguments"] = picojson::value(arguments);

	const bool sent = request("tools/call", params, [completion](const McpToolResult& raw) {
		if (completion) completion(raw.ok ? toolResult(raw.value) : raw);
	});
	if (!sent && completion) deliver(completion, { false, {}, "task server is not running" });
}

McpToolResult McpClient::callToolBlocking(const std::string& name, picojson::object arguments) {
	bool done = false;
	McpToolResult outcome;
	callTool(name, std::move(arguments), [&](const McpToolResult& result) {
		outcome = result;
		done = true;
	});

	if (!waitUntil([&] { return done; })) return { false, {}, "task server stopped" };
	return outcome;
}

void McpClient::readLoop() {
	std::string buffer;
	char chunk[4096];

	while (true) {
		const ssize_t count = ::read(mFromServer, chunk, sizeof(chunk));
		if (count < 0 && errno == EINTR) continue;
		if (count <= 0) break;

		buffer.append(chunk, size_t(count));
		for (size_t newline = buffer.find('\n'); newline != std::string::npos; newline = buffer.find('\n')) {
			const std::string line = buffer.substr(0, newline);
			buffer.erase(0, newline + 1);
			if (!line.empty()) handleMessage(line);
		}
	}

	{
		std::lock_guard lock(mMutex);
		mRunning = false;
	}
	failAll("task server exited");
}

void McpClient::handleMessage(const std::string& line) {
	picojson::value message;
	if (!picojson::parse(message, line).empty() || !message.is<picojson::object>()) return;

	const picojson::object& object = message.get<picojson::object>();
	auto id = object.find("id");
	if (id == object.end() || !id->second.is<double>()) return;

	Completion completion;
	{
		std::lock_guard lock(mMutex);
		auto pending = mPending.find(long(id->second.get<double>()));
		if (pending == mPending.end()) return;
		completion = std::move(pending->second.completion);
		mPending.erase(pending);
	}

	auto error = object.find("error");
	if (error != object.end()) {
		deliver(completion, { false, {}, error->second.serialize() });
		return;
	}
	auto result = object.find("result");
	deliver(completion, { true, result == object.end() ? picojson::value() : result->second, {} });
}

size_t McpClient::pendingCount() const {
	std::lock_guard lock(mMutex);
	return mPending.size() + mReady.size();
}

void McpClient::waitForIdle() {
	waitUntil([this] { return pendingCount() == 0; });
}

void McpClient::setOnResponse(std::function<void()> callback) {
	std::lock_guard lock(mMutex);
	mOnResponse = std::move(callback);
}

void McpClient::deliver(Completion completion, McpToolResult result) {
	std::function<void()> onResponse;
	{
		std::lock_guard lock(mMutex);
		mReady.emplace_back(std::move(completion), std::move(result));
		onResponse = mOnResponse;
		mResponded.notify_all();
	}
	if (onResponse) onResponse();
}

void McpClient::failAll(const std::string& reason) {
	std::map<long, Pending> pending;
	{
		std::lock_guard lock(mMutex);
		pending.swap(mPending);
	}
	for (auto& [id, entry] : pending)
		deliver(std::move(entry.completion), { false, {}, reason });

	std::lock_guard lock(mMutex);
	mResponded.notify_all();
}

void McpClient::dispatchCompletions() {
	std::deque<std::pair<Completion, McpToolResult>> ready;
	{
		std::lock_guard lock(mMutex);
		ready.swap(mReady);
	}
	for (auto& [completion, result] : ready) {
		if (completion) completion(result);
	}
}

McpToolResult McpClient::toolResult(const picojson::value& response) {
	if (!response.is<picojson::object>()) return { false, {}, "malformed tool response" };
	const picojson::object& object = response.get<picojson::object>();

	auto isError = object.find("isError");
	auto content = object.find("content");
	std::string text;
	if (
		content != object.end() && content->second.is<picojson::array>() && !content->second.get<picojson::array>().empty()
	) {
		const picojson::value& first = content->second.get<picojson::array>().front();
		if (first.is<picojson::object>() && first.get<picojson::object>().count("text"))
			text = first.get<picojson::object>().at("text").to_str();
	}

	if (isError != object.end() && isError->second.evaluate_as_boolean()) return { false, {}, text };

	auto structured = object.find("structuredContent");
	if (structured != object.end() && structured->second.is<picojson::object>()) {
		const picojson::object& wrapped = structured->second.get<picojson::object>();
		auto inner = wrapped.find("result");
		return { true, inner == wrapped.end() ? structured->second : inner->second, {} };
	}

	picojson::value parsed;
	if (!text.empty() && picojson::parse(parsed, text).empty()) return { true, parsed, {} };
	return { true, picojson::value(text), {} };
}

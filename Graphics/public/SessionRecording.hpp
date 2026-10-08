#pragma once

#include "InputRecording.hpp"

#include <functional>
#include <memory>
#include <string>

namespace tp {

	struct SessionEnvironment {
		std::string revision;
		std::string device;
		std::string date;
		Vec2F windowSize = { 0, 0 };
		halnf contentScale = 1;
		std::string application;

		[[nodiscard]] std::string serialize() const;
	};

	struct SessionRecording {
		using Saver = std::function<bool(const std::string& directory)>;

		SessionEnvironment environment;
		InputRecording input;
		std::string recordedFrames;
		Saver saveSnapshot;

		bool save(const std::string& directory) const;
	};

	class SessionRecorderHost {
	public:
		virtual ~SessionRecorderHost() = default;

		virtual void setRawInputTap(WindowInputObserver* tap) = 0;
		[[nodiscard]] virtual SessionEnvironment environment() const = 0;
		[[nodiscard]] virtual double nowMs() const = 0;
	};

	class SessionRecorder {
	public:
		using Snapshot = std::function<SessionRecording::Saver(const SessionEnvironment& environment)>;

		explicit SessionRecorder(SessionRecorderHost& host);
		~SessionRecorder();

		SessionRecorder(const SessionRecorder&) = delete;
		SessionRecorder& operator=(const SessionRecorder&) = delete;

		void setSnapshot(Snapshot snapshot) { mSnapshot = std::move(snapshot); }

		[[nodiscard]] bool isRecording() const { return mRecording != nullptr; }
		[[nodiscard]] double elapsedMs() const;

		void start();
		[[nodiscard]] SessionRecording stop();
		void cancel();

		[[nodiscard]] static std::string defaultDirectory(const std::string& applicationSessionsDirectory);
		[[nodiscard]] static std::string recordedFramesTsv();

	private:
		struct Recording;

		SessionRecorderHost& mHost;
		Snapshot mSnapshot;
		std::unique_ptr<Recording> mRecording;
	};
}

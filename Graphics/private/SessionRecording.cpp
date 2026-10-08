#include "SessionRecording.hpp"

#include "Profiler.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace tp {

	namespace {
		bool writeText(const std::filesystem::path& path, const std::string& text) {
			std::ofstream file(path, std::ios::binary | std::ios::trunc);
			file.write(text.data(), (std::streamsize) text.size());
			return (bool) file;
		}
	}

	std::string SessionEnvironment::serialize() const {
		char size[64];
		std::snprintf(size, sizeof(size), "%.1f %.1f", windowSize.x, windowSize.y);
		char scale[32];
		std::snprintf(scale, sizeof(scale), "%.3f", contentScale);
		return "application " + application + "\nrevision " + revision + "\ndevice " + device + "\ndate " + date +
					 "\nwindow " + size + "\nscale " + scale + "\n";
	}

	bool SessionRecording::save(const std::string& directory) const {
		const std::filesystem::path root(directory);
		std::error_code error;
		std::filesystem::create_directories(root, error);
		if (error) return false;

		bool written = writeText(root / "environment.txt", environment.serialize());
		written = written && input.save((root / "input.txt").string());
		if (!recordedFrames.empty()) written = written && writeText(root / "recorded-frames.tsv", recordedFrames);
		if (saveSnapshot) written = written && saveSnapshot(directory);
		return written;
	}

	struct SessionRecorder::Recording {
		explicit Recording(InputRecorder::Clock clock) :
			input(std::move(clock)) {}

		SessionEnvironment environment;
		SessionRecording::Saver snapshot;
		InputRecorder input;
		double startMs = 0;
		bool profilerWasEnabled = false;
	};

	SessionRecorder::SessionRecorder(SessionRecorderHost& host) :
		mHost(host) {}

	SessionRecorder::~SessionRecorder() { cancel(); }

	double SessionRecorder::elapsedMs() const { return mRecording ? mHost.nowMs() - mRecording->startMs : 0.0; }

	void SessionRecorder::start() {
		if (mRecording) return;

		const SessionEnvironment environment = mHost.environment();
		SessionRecording::Saver snapshot = mSnapshot ? mSnapshot(environment) : SessionRecording::Saver();
		mRecording = std::make_unique<Recording>([this]() { return mHost.nowMs(); });
		mRecording->startMs = mHost.nowMs();
		mRecording->environment = environment;
		mRecording->snapshot = std::move(snapshot);
		mRecording->input.setWindowSize(environment.windowSize);
		mRecording->input.startClockNow();

		Profiler& profiler = Profiler::global();
		mRecording->profilerWasEnabled = profiler.isEnabled();
		profiler.clear();
		profiler.setEnabled(true);
		mHost.setRawInputTap(&mRecording->input);
	}

	SessionRecording SessionRecorder::stop() {
		SessionRecording session;
		if (!mRecording) return session;

		mHost.setRawInputTap(nullptr);
		session.environment = std::move(mRecording->environment);
		session.input = mRecording->input.recording();
		session.recordedFrames = recordedFramesTsv();
		session.saveSnapshot = std::move(mRecording->snapshot);

		Profiler& profiler = Profiler::global();
		profiler.setEnabled(mRecording->profilerWasEnabled);
		profiler.clear();
		mRecording.reset();
		return session;
	}

	void SessionRecorder::cancel() {
		if (!mRecording) return;
		mHost.setRawInputTap(nullptr);
		Profiler::global().setEnabled(mRecording->profilerWasEnabled);
		Profiler::global().clear();
		mRecording.reset();
	}

	std::string SessionRecorder::defaultDirectory(const std::string& applicationSessionsDirectory) {
		const char* configured = std::getenv("MODULES_SESSION_DIR");
		if (configured && *configured) return configured;
		return applicationSessionsDirectory;
	}

	std::string SessionRecorder::recordedFramesTsv() {
		const Profiler& profiler = Profiler::global();
		std::string text = "frame\twall_ms\tcpu_ms";
		for (const std::string& name : profiler.sectionNames())
			text += "\t" + name;
		for (const std::string& name : profiler.gpuSectionNames())
			text += "\t" + name;
		for (const std::string& name : profiler.counterNames())
			text += "\t" + name;
		text += "\n";

		char value[32];
		auto append = [&](double number) {
			std::snprintf(value, sizeof(value), "\t%.4f", number);
			text += value;
		};
		for (const FrameProfile& frame : profiler.frames()) {
			text += std::to_string(frame.index);
			append(frame.wallMs);
			append(frame.cpuMs);
			for (size_t index = 0; index < profiler.sectionNames().size(); index++)
				append(index < frame.sectionMs.size() ? frame.sectionMs[index] : 0.0);
			for (size_t index = 0; index < profiler.gpuSectionNames().size(); index++)
				append(index < frame.gpuMs.size() ? frame.gpuMs[index] : 0.0);
			for (size_t index = 0; index < profiler.counterNames().size(); index++)
				append(index < frame.counters.size() ? frame.counters[index] : 0.0);
			text += "\n";
		}
		return text;
	}
}

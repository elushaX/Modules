#include "InputRecording.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace tp {

	namespace {
		const char* const kKindNames[] = { "sample", "move", "leave", "button", "scroll", "key", "focus" };

		std::optional<RecordedInput::Kind> kindNamed(const std::string& name) {
			for (size_t index = 0; index < std::size(kKindNames); index++) {
				if (name == kKindNames[index]) return (RecordedInput::Kind) index;
			}
			return std::nullopt;
		}

		double steadyMs() {
			const auto now = std::chrono::steady_clock::now().time_since_epoch();
			return std::chrono::duration<double, std::milli>(now).count();
		}

		bool isPointerKind(RecordedInput::Kind kind) {
			return kind == RecordedInput::Kind::Sample || kind == RecordedInput::Kind::Move ||
						 kind == RecordedInput::Kind::Button || kind == RecordedInput::Kind::Leave;
		}
	}

	void RecordedInput::deliverTo(WindowInputObserver& observer) const {
		switch (kind) {
			case Kind::Sample: observer.onPointerSample(sample); break;
			case Kind::Move: observer.onPointerMoved(vector); break;
			case Kind::Leave: observer.onPointerLeft(); break;
			case Kind::Button: observer.onPointerButton(id, flag); break;
			case Kind::Scroll: observer.onScrolled(vector); break;
			case Kind::Key: observer.onKey(id, flag); break;
			case Kind::Focus: observer.onKeyboardFocus(flag); break;
		}
	}

	std::string InputRecording::serialize() const {
		std::string text = std::string(kHeader) + "\n";
		char line[256];
		std::snprintf(line, sizeof(line), "window %.1f %.1f\n", windowSize.x, windowSize.y);
		text += line;

		for (const RecordedInput& event : events) {
			const char* kind = kKindNames[(size_t) event.kind];
			switch (event.kind) {
				case RecordedInput::Kind::Sample:
					std::snprintf(
						line,
						sizeof(line),
						"%.3f %s %d %.5f %.4f %.4f %.4f %.4f %u\n",
						event.timeMs,
						kind,
						(int) event.sample.tool,
						event.sample.pressure,
						event.sample.tilt.x,
						event.sample.tilt.y,
						event.sample.rotation,
						event.sample.distance,
						event.sample.timeMs
					);
					break;
				case RecordedInput::Kind::Move:
				case RecordedInput::Kind::Scroll:
					std::snprintf(line, sizeof(line), "%.3f %s %.3f %.3f\n", event.timeMs, kind, event.vector.x, event.vector.y);
					break;
				case RecordedInput::Kind::Button:
				case RecordedInput::Kind::Key:
					std::snprintf(
						line, sizeof(line), "%.3f %s %llu %d\n", event.timeMs, kind, (unsigned long long) event.id, (int) event.flag
					);
					break;
				case RecordedInput::Kind::Focus:
					std::snprintf(line, sizeof(line), "%.3f %s %d\n", event.timeMs, kind, (int) event.flag);
					break;
				case RecordedInput::Kind::Leave: std::snprintf(line, sizeof(line), "%.3f %s\n", event.timeMs, kind); break;
			}
			text += line;
		}
		return text;
	}

	std::optional<InputRecording> InputRecording::parse(const std::string& text) {
		std::istringstream input(text);
		std::string line;
		if (!std::getline(input, line) || line != kHeader) return std::nullopt;

		InputRecording recording;
		while (std::getline(input, line)) {
			if (line.empty()) continue;
			std::istringstream fields(line);
			std::string first;
			fields >> first;
			if (first == "window") {
				fields >> recording.windowSize.x >> recording.windowSize.y;
				continue;
			}

			RecordedInput event;
			std::string kindName;
			event.timeMs = std::stod(first);
			fields >> kindName;
			const std::optional<RecordedInput::Kind> kind = kindNamed(kindName);
			if (!kind) return std::nullopt;
			event.kind = *kind;

			unsigned long long id = 0;
			int flag = 0;
			int tool = 0;
			switch (event.kind) {
				case RecordedInput::Kind::Sample:
					fields >> tool >> event.sample.pressure >> event.sample.tilt.x >> event.sample.tilt.y >>
						event.sample.rotation >> event.sample.distance >> event.sample.timeMs;
					event.sample.tool = (PointerTool) tool;
					break;
				case RecordedInput::Kind::Move:
				case RecordedInput::Kind::Scroll: fields >> event.vector.x >> event.vector.y; break;
				case RecordedInput::Kind::Button:
				case RecordedInput::Kind::Key:
					fields >> id >> flag;
					event.id = (InputID) id;
					event.flag = flag != 0;
					break;
				case RecordedInput::Kind::Focus:
					fields >> flag;
					event.flag = flag != 0;
					break;
				case RecordedInput::Kind::Leave: break;
			}
			if (fields.fail()) return std::nullopt;
			recording.events.push_back(event);
		}
		return recording;
	}

	bool InputRecording::save(const std::string& path) const {
		std::ofstream file(path);
		if (!file) return false;
		file << serialize();
		return (bool) file;
	}

	std::optional<InputRecording> InputRecording::load(const std::string& path) {
		std::ifstream file(path);
		if (!file) return std::nullopt;
		std::stringstream text;
		text << file.rdbuf();
		return parse(text.str());
	}

	InputRecording InputRecording::translated(Vec2F offset) const {
		InputRecording moved = *this;
		for (RecordedInput& event : moved.events) {
			if (event.kind == RecordedInput::Kind::Move) event.vector += offset;
		}
		return moved;
	}

	InputRecording InputRecording::slicedToPointerActivity() const {
		InputRecording sliced;
		sliced.windowSize = windowSize;

		size_t first = events.size();
		size_t last = 0;
		for (size_t index = 0; index < events.size(); index++) {
			if (events[index].kind != RecordedInput::Kind::Button) continue;
			first = std::min(first, index);
			last = index;
		}
		if (first >= events.size()) return sliced;

		while (first > 0 && isPointerKind(events[first - 1].kind) && events[first - 1].kind != RecordedInput::Kind::Button)
			first--;
		const double origin = events[first].timeMs;
		for (size_t index = first; index <= last; index++) {
			if (!isPointerKind(events[index].kind)) continue;
			RecordedInput event = events[index];
			event.timeMs -= origin;
			sliced.events.push_back(event);
		}
		return sliced;
	}

	InputRecorder::InputRecorder(Clock clock) :
		mClock(clock ? std::move(clock) : Clock(steadyMs)) {}

	void InputRecorder::record(RecordedInput event) {
		const double now = mClock();
		if (mStartMs < 0) mStartMs = now;
		event.timeMs = now - mStartMs;
		mRecording.events.push_back(event);
	}

	void InputRecorder::onPointerSample(const PointerSample& sample) {
		RecordedInput event;
		event.kind = RecordedInput::Kind::Sample;
		event.sample = sample;
		record(event);
	}

	void InputRecorder::onPointerMoved(Vec2F contentPosition) {
		RecordedInput event;
		event.kind = RecordedInput::Kind::Move;
		event.vector = contentPosition;
		record(event);
	}

	void InputRecorder::onPointerLeft() {
		RecordedInput event;
		event.kind = RecordedInput::Kind::Leave;
		record(event);
	}

	void InputRecorder::onPointerButton(InputID button, bool pressed) {
		RecordedInput event;
		event.kind = RecordedInput::Kind::Button;
		event.id = button;
		event.flag = pressed;
		record(event);
	}

	void InputRecorder::onScrolled(Vec2F delta) {
		RecordedInput event;
		event.kind = RecordedInput::Kind::Scroll;
		event.vector = delta;
		record(event);
	}

	void InputRecorder::onKey(InputID key, bool pressed) {
		RecordedInput event;
		event.kind = RecordedInput::Kind::Key;
		event.id = key;
		event.flag = pressed;
		record(event);
	}

	void InputRecorder::onKeyboardFocus(bool focused) {
		RecordedInput event;
		event.kind = RecordedInput::Kind::Focus;
		event.flag = focused;
		record(event);
	}
}

#pragma once

#include "WindowInput.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tp {

	struct RecordedInput {
		enum class Kind { Sample, Move, Leave, Button, Scroll, Key, Focus };

		double timeMs = 0;
		Kind kind = Kind::Move;
		Vec2F vector = { 0, 0 };
		InputID id = InputID::MOUSE1;
		bool flag = false;
		PointerSample sample;

		void deliverTo(WindowInputObserver& observer) const;
		[[nodiscard]] bool operator==(const RecordedInput& other) const = default;
	};

	struct InputRecording {
		static constexpr const char* kHeader = "tp-input-recording 1";

		Vec2F windowSize = { 0, 0 };
		std::vector<RecordedInput> events;

		[[nodiscard]] double durationMs() const { return events.empty() ? 0.0 : events.back().timeMs; }
		[[nodiscard]] std::string serialize() const;
		[[nodiscard]] static std::optional<InputRecording> parse(const std::string& text);

		bool save(const std::string& path) const;
		[[nodiscard]] static std::optional<InputRecording> load(const std::string& path);

		[[nodiscard]] InputRecording translated(Vec2F offset) const;
		[[nodiscard]] InputRecording slicedToPointerActivity() const;
	};

	class InputRecorder : public WindowInputObserver {
	public:
		using Clock = std::function<double()>;

		explicit InputRecorder(Clock clock = {});

		void setWindowSize(Vec2F size) { mRecording.windowSize = size; }
		void startClockNow() { mStartMs = mClock(); }
		[[nodiscard]] const InputRecording& recording() const { return mRecording; }

		void onPointerSample(const PointerSample& sample) override;
		void onPointerMoved(Vec2F contentPosition) override;
		void onPointerLeft() override;
		void onPointerButton(InputID button, bool pressed) override;
		void onScrolled(Vec2F delta) override;
		void onKey(InputID key, bool pressed) override;
		void onKeyboardFocus(bool focused) override;

	private:
		void record(RecordedInput event);

		Clock mClock;
		double mStartMs = -1;
		InputRecording mRecording;
	};
}

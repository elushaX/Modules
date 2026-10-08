#include "BenchInput.hpp"

#include <algorithm>
#include <cmath>

namespace tp::sketch2d::bench {

	namespace {
		constexpr double kPi = 3.14159265358979;

		class Random {
		public:
			explicit Random(uint32_t seed) :
				mState(seed ? seed : 1) {}

			double next() {
				mState ^= mState << 13;
				mState ^= mState >> 17;
				mState ^= mState << 5;
				return (double) mState / 4294967296.0;
			}

			double between(double low, double high) { return low + (high - low) * next(); }

		private:
			uint32_t mState;
		};

		struct Pen {
			Vec2F position;
			double heading = 0;
			double turnRate = 0;
		};

		PointerSample sampleAt(bool pen, double strokeFraction, double timeMs, double heading) {
			PointerSample sample;
			if (!pen) return sample;
			sample.tool = PointerTool::Pen;
			const double rise = std::min(1.0, strokeFraction * 6.0);
			const double fall = std::min(1.0, (1.0 - strokeFraction) * 5.0);
			sample.pressure = (halnf) std::clamp(0.25 + 0.7 * rise * fall, 0.05, 1.0);
			sample.tilt = { (halnf) (0.3 * std::cos(heading)), (halnf) (0.3 * std::sin(heading)) };
			sample.timeMs = (uint32_t) timeMs;
			return sample;
		}

		void keepInside(Pen& pen, const RectF& area) {
			const Vec2F center = area.center();
			const Vec2F half = area.size * 0.5f;
			const Vec2F offset = pen.position - center;
			if (std::fabs(offset.x) < half.x * 0.85f && std::fabs(offset.y) < half.y * 0.85f) return;
			const double towardCenter = std::atan2(-offset.y, -offset.x);
			double difference = towardCenter - pen.heading;
			while (difference > kPi)
				difference -= 2 * kPi;
			while (difference < -kPi)
				difference += 2 * kPi;
			pen.heading += difference * 0.2;
			pen.position.x = std::clamp(pen.position.x, area.pos.x, area.pos.x + area.size.x);
			pen.position.y = std::clamp(pen.position.y, area.pos.y, area.pos.y + area.size.y);
		}

		void push(InputRecording& recording, double timeMs, RecordedInput::Kind kind) {
			RecordedInput event;
			event.timeMs = timeMs;
			event.kind = kind;
			recording.events.push_back(event);
		}
	}

	InputRecording fastScribble(const ScribbleOptions& options) {
		InputRecording recording;
		Random random(options.seed);
		Pen pen;
		pen.position = options.area.center();
		pen.heading = random.between(0, 2 * kPi);

		double time = 0;
		while (time < options.durationMs) {
			const double strokeEnd = std::min(time + options.strokeMs, options.durationMs);
			const double strokeLength = strokeEnd - time;
			bool down = false;

			for (double now = time; now <= strokeEnd; now += options.sampleIntervalMs) {
				const double fraction = strokeLength > 0 ? (now - time) / strokeLength : 1.0;
				if (random.next() < 0.04) pen.turnRate = random.between(-0.35, 0.35);
				pen.heading += pen.turnRate + random.between(-0.05, 0.05);
				const double step = options.speedPixelsPerSecond * options.sampleIntervalMs / 1000.0;
				pen.position += Vec2F((halnf) (std::cos(pen.heading) * step), (halnf) (std::sin(pen.heading) * step));
				keepInside(pen, options.area);

				RecordedInput sample;
				sample.timeMs = now;
				sample.kind = RecordedInput::Kind::Sample;
				sample.sample = sampleAt(options.pen, fraction, now, pen.heading);
				recording.events.push_back(sample);

				RecordedInput move;
				move.timeMs = now;
				move.kind = RecordedInput::Kind::Move;
				move.vector = pen.position;
				recording.events.push_back(move);

				if (!down) {
					RecordedInput press;
					press.timeMs = now;
					press.kind = RecordedInput::Kind::Button;
					press.id = InputID::MOUSE1;
					press.flag = true;
					recording.events.push_back(press);
					down = true;
				}
			}

			RecordedInput release;
			release.timeMs = strokeEnd;
			release.kind = RecordedInput::Kind::Button;
			release.id = InputID::MOUSE1;
			release.flag = false;
			recording.events.push_back(release);
			if (options.pen) {
				RecordedInput away;
				away.timeMs = strokeEnd;
				away.kind = RecordedInput::Kind::Sample;
				recording.events.push_back(away);
			}
			time = strokeEnd + options.gapMs;
		}
		push(recording, time, RecordedInput::Kind::Leave);
		return recording;
	}
}

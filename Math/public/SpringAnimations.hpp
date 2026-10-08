#pragma once

#include "Color.hpp"
#include "Rect.hpp"

#include <cmath>

namespace tp {

	struct SpringParameters {
		halnf stiffness = 0.0008f;
		halnf damping = 0.0566f;

		static SpringParameters criticallyDamped(halnf responseMs) {
			const halnf angularFrequency = 2.0f * 3.14159265f / responseMs;
			return { angularFrequency * angularFrequency, 2.0f * angularFrequency };
		}
	};

	class SpringValue {
	public:
		SpringValue() = default;

		explicit SpringValue(halnf value) :
			mValue(value),
			mTarget(value) {}

		void setParameters(const SpringParameters& parameters) { mParameters = parameters; }

		void setTarget(halnf target) { mTarget = target; }

		void snapTo(halnf value) {
			mValue = value;
			mTarget = value;
			mVelocity = 0;
		}

		void snapToTarget() { snapTo(mTarget); }

		void launch(halnf value, halnf velocity, halnf target) {
			mValue = value;
			mVelocity = velocity;
			mTarget = target;
		}

		void step(halnf deltaMs) {
			while (deltaMs > 0) {
				const halnf substep = deltaMs < kMaxSubstepMs ? deltaMs : kMaxSubstepMs;
				const halnf acceleration = mParameters.stiffness * (mTarget - mValue) - mParameters.damping * mVelocity;

				mVelocity += acceleration * substep;
				mValue += mVelocity * substep;
				deltaMs -= substep;
			}
		}

		[[nodiscard]] bool isSettled(halnf tolerance) const {
			const bool closeToTarget = std::fabs(mTarget - mValue) <= tolerance;
			const bool nearlyStill = std::fabs(mVelocity) * kVelocityHorizonMs <= tolerance;
			return closeToTarget && nearlyStill;
		}

		[[nodiscard]] halnf value() const { return mValue; }
		[[nodiscard]] halnf target() const { return mTarget; }
		[[nodiscard]] halnf velocity() const { return mVelocity; }

	private:
		static constexpr halnf kMaxSubstepMs = 1.0f;
		static constexpr halnf kVelocityHorizonMs = 16.0f;

		SpringParameters mParameters;
		halnf mValue = 0;
		halnf mTarget = 0;
		halnf mVelocity = 0;
	};

	template <int ChannelCount>
	class SpringChannels {
	public:
		void setParameters(const SpringParameters& parameters) {
			for (auto& channel : mChannels)
				channel.setParameters(parameters);
		}

		void step(halnf deltaMs) {
			for (auto& channel : mChannels)
				channel.step(deltaMs);
		}

		void snapToTarget() {
			for (auto& channel : mChannels)
				channel.snapToTarget();
		}

		[[nodiscard]] bool isSettled(halnf tolerance) const {
			for (const auto& channel : mChannels) {
				if (!channel.isSettled(tolerance)) return false;
			}
			return true;
		}

	protected:
		void setChannelTargets(const halnf (&targets)[ChannelCount]) {
			for (int index = 0; index < ChannelCount; index++)
				mChannels[index].setTarget(targets[index]);
		}

		void snapChannels(const halnf (&values)[ChannelCount]) {
			for (int index = 0; index < ChannelCount; index++)
				mChannels[index].snapTo(values[index]);
		}

		[[nodiscard]] halnf channelValue(int index) const { return mChannels[index].value(); }
		[[nodiscard]] halnf channelTarget(int index) const { return mChannels[index].target(); }

	private:
		SpringValue mChannels[ChannelCount];
	};

	class SpringVec2 : public SpringChannels<2> {
	public:
		void setTarget(const Vec2F& target) { setChannelTargets({ target.x, target.y }); }
		void snapTo(const Vec2F& value) { snapChannels({ value.x, value.y }); }

		[[nodiscard]] Vec2F value() const { return { channelValue(0), channelValue(1) }; }
		[[nodiscard]] Vec2F target() const { return { channelTarget(0), channelTarget(1) }; }
	};

	class SpringRect : public SpringChannels<4> {
	public:
		void setTarget(const RectF& target) { setChannelTargets({ target.x, target.y, target.z, target.w }); }
		void snapTo(const RectF& value) { snapChannels({ value.x, value.y, value.z, value.w }); }

		[[nodiscard]] RectF value() const { return { channelValue(0), channelValue(1), channelValue(2), channelValue(3) }; }

		[[nodiscard]] RectF target() const {
			return { channelTarget(0), channelTarget(1), channelTarget(2), channelTarget(3) };
		}
	};

	class SpringColor : public SpringChannels<4> {
	public:
		void setTarget(const RGBA& target) { setChannelTargets({ target.r, target.g, target.b, target.a }); }
		void snapTo(const RGBA& value) { snapChannels({ value.r, value.g, value.b, value.a }); }

		[[nodiscard]] RGBA value() const { return { channelValue(0), channelValue(1), channelValue(2), channelValue(3) }; }

		[[nodiscard]] RGBA target() const {
			return { channelTarget(0), channelTarget(1), channelTarget(2), channelTarget(3) };
		}
	};
}

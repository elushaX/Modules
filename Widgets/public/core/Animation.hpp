#pragma once

#include "SpringAnimations.hpp"

namespace tp::ui {

	struct Motion {
		static SpringParameters quick() { return SpringParameters::criticallyDamped(160); }
		static SpringParameters smooth() { return SpringParameters::criticallyDamped(320); }
		static SpringParameters fluid() { return { 0.0041f, 0.25f }; }
	};

	template <typename Spring, typename Value>
	class Animated {
	public:
		explicit Animated(
			const Value& initial, const SpringParameters& parameters = Motion::quick(), halnf settleTolerance = 0.002f
		) :
			mSettleTolerance(settleTolerance) {
			mSpring.setParameters(parameters);
			mSpring.snapTo(initial);
		}

		void animateTo(const Value& target) { mSpring.setTarget(target); }
		void jumpTo(const Value& value) { mSpring.snapTo(value); }
		void finish() { mSpring.snapToTarget(); }
		void setParameters(const SpringParameters& parameters) { mSpring.setParameters(parameters); }
		void launch(const Value& value, halnf velocity, const Value& target)
			requires requires(Spring spring) { spring.launch(value, velocity, target); }
		{
			mSpring.launch(value, velocity, target);
		}
		[[nodiscard]] halnf velocity() const
			requires requires(const Spring spring) { spring.velocity(); }
		{
			return mSpring.velocity();
		}

		bool advance(halnf deltaMs) {
			mSpring.step(deltaMs);
			if (mSpring.isSettled(mSettleTolerance)) mSpring.snapToTarget();
			return isAnimating();
		}

		[[nodiscard]] bool isAnimating() const { return !mSpring.isSettled(0); }
		[[nodiscard]] Value value() const { return mSpring.value(); }
		[[nodiscard]] Value target() const { return mSpring.target(); }

	private:
		halnf mSettleTolerance;
		Spring mSpring;
	};

	class AnimatedFloat : public Animated<SpringValue, halnf> {
	public:
		using Animated::Animated;
	};

	class AnimatedColor : public Animated<SpringColor, RGBA> {
	public:
		using Animated::Animated;
	};

	class AnimatedRect : public Animated<SpringRect, RectF> {
	public:
		static constexpr halnf kGeometryTolerance = 0.25f;

		using Animated::Animated;
	};
}

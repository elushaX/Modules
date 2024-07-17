#pragma once

#include "Widget.hpp"

namespace tp {
	class AnimationTestWidget : public Widget {

	public:
		AnimationTestWidget() = default;
		~AnimationTestWidget() override = default;

		void draw(Canvas& canvas) override;
		void process(const EventHandler& events) override;

		[[nodiscard]] bool needUpdate() const override;

	private:
		SpringRect mTestSpring;
	};
}
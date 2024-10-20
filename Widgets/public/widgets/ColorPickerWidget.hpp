#pragma once

#include "Widget.hpp"

namespace tp {
	class RGBPickerWidget : public Widget {
	public:
		RGBPickerWidget() = default;

		void process(const EventHandler& events) override;
		void draw(Canvas& canvas) override;

		[[nodiscard]] bool processesEvents() const override { return true; }

	public:
		Canvas::ColorWheel mColorWheel;
	};
}
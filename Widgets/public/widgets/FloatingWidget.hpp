#pragma once

#include "SimpleWidgets.hpp"
#include "FloatingLayout.hpp"
#include "ScrollableWidget.hpp"

namespace tp {
	class FloatingWidget : public Widget {
	public:
		FloatingWidget() : Widget() {
			setDebug("float", { 0.0, 0.9, 0.1, 1 });
			setLayout(new FloatingLayout(this));
		}

		void process(const EventHandler& events) override;
		void draw(Canvas& canvas) override;

		[[nodiscard]] bool needsNextFrame() const override;

		[[nodiscard]] bool propagateEventsToChildren() const override;
		[[nodiscard]] bool processesEvents() const override;

		[[nodiscard]] bool isFloating() const;

	private:
		FloatingLayout* layout();
		[[nodiscard]] const FloatingLayout* layout() const;
	};
}
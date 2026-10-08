#pragma once

#include "Event.hpp"
#include "Painter.hpp"
#include "Widget.hpp"

#include <string>
#include <vector>

namespace tp::ui::testing {

	class Box : public Widget {
	public:
		explicit Box(Vec2F preferred = { 0, 0 }, std::string name = "box") :
			mPreferred(preferred) {
			setDebugName(std::move(name));
		}

		void setPreferred(Vec2F preferred) {
			mPreferred = preferred;
			requestLayout();
		}

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf) const override {
			return Extent::shrinkable(along(mPreferred, axis));
		}

	private:
		Vec2F mPreferred;
	};

	class EventProbe : public Box {
	public:
		explicit EventProbe(Vec2F preferred = { 50, 30 }, std::string name = "probe") :
			Box(preferred, std::move(name)) {}

		std::vector<Event::Type> received;
		std::vector<Vec2F> localPositions;
		int hoverChanges = 0;
		int ticks = 0;
		int paints = 0;
		bool acceptEvents = true;
		bool focusable = false;

		[[nodiscard]] int count(Event::Type type) const {
			int total = 0;
			for (Event::Type receivedType : received)
				total += receivedType == type ? 1 : 0;
			return total;
		}

		void beginTicking() { startTicking(); }
		void endTicking() { stopTicking(); }

	protected:
		void onEvent(Event& event) override {
			received.push_back(event.type());
			localPositions.push_back(event.position());
			if (acceptEvents) event.accept();
		}

		void onHoverChanged() override { hoverChanges++; }
		void onTick(halnf) override { ticks++; }
		void paint(Painter& painter) override {
			paints++;
			painter.fillRect(localRect(), RGBA(1, 0, 0, 1));
		}
		[[nodiscard]] bool acceptsKeyboardFocus() const override { return focusable; }
	};
}

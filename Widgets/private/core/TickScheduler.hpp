#pragma once

#include "Environment.hpp"

#include <vector>

namespace tp::ui {

	class Widget;

	class TickScheduler {
	public:
		void start(Widget& widget);
		void stop(const Widget& widget);
		void startGeometry(Widget& widget);
		void forget(const Widget& widget);

		void advance(halnf deltaMs);
		[[nodiscard]] bool isActive() const { return !mTicking.empty() || !mAnimatingGeometry.empty(); }
		[[nodiscard]] const std::vector<Widget*>& ticking() const { return mTicking; }
		[[nodiscard]] const std::vector<Widget*>& animatingGeometry() const { return mAnimatingGeometry; }

	private:
		static void insertUnique(std::vector<Widget*>& widgets, Widget& widget);
		static void erase(std::vector<Widget*>& widgets, const Widget& widget);

		void advanceTicks(halnf deltaMs);
		void advanceGeometry(halnf deltaMs);

		std::vector<Widget*> mTicking;
		std::vector<Widget*> mAnimatingGeometry;
	};
}

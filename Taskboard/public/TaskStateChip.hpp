#pragma once

#include "Task.hpp"
#include "Widget.hpp"

#include <string>

namespace tp::taskboard {

	class TaskStateChip : public ui::Widget {
	public:
		TaskStateChip();

		void show(std::string text, const RGBA& color, bool pulsing);
		void setDotShown(bool shown);
		[[nodiscard]] bool isDotShown() const { return mDotShown; }
		[[nodiscard]] const std::string& text() const { return mText; }
		[[nodiscard]] const RGBA& color() const { return mColor; }
		[[nodiscard]] bool isPulsing() const { return mPulsing; }
		void showSession(SessionState state);

	protected:
		[[nodiscard]] ui::Extent measureContent(ui::Axis axis, halnf crossLength) const override;
		void paint(ui::Painter& painter) override;
		void onTick(halnf deltaMs) override;

	private:
		[[nodiscard]] halnf textStart() const;

		std::string mText;
		RGBA mColor;
		bool mPulsing = false;
		bool mDotShown = true;
		halnf mClockMs = 0;
	};
}

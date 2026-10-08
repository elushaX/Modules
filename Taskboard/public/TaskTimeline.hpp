#pragma once

#include "Button.hpp"
#include "Label.hpp"
#include "MediaControls.hpp"
#include "Menus.hpp"
#include "Slider.hpp"

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tp::taskboard {

	[[nodiscard]] std::string formatMoment(double unixSeconds);

	struct TimelinePrecision {
		const char* label;
		double window;
		double step;
	};

	inline constexpr std::array kTimelinePrecisions = {
		TimelinePrecision{ "1 minute", 60, 1 },
		TimelinePrecision{ "1 hour", 60 * 60, 60 },
		TimelinePrecision{ "1 day", 24 * 60 * 60, 15 * 60 },
		TimelinePrecision{ "1 week", 7 * 24 * 60 * 60, 60 * 60 },
	};

	class TaskTimeline : public ui::Widget {
	public:
		TaskTimeline();

		void setSpan(std::optional<double> first, std::optional<double> last);
		void setChangeMoments(std::vector<double> moments);
		void showMoment(std::optional<double> moment);
		void setPrecision(size_t index);

		[[nodiscard]] std::optional<double> shownMoment() const { return mMoment; }
		[[nodiscard]] const TimelinePrecision& precision() const { return kTimelinePrecisions[mPrecision]; }
		[[nodiscard]] size_t stepCount() const;
		[[nodiscard]] double windowStart() const;
		[[nodiscard]] double windowEnd() const;
		[[nodiscard]] std::optional<double> momentAtStep(size_t step) const;
		[[nodiscard]] size_t stepOf(std::optional<double> moment) const;

		void chooseStep(size_t step);
		void previousWindow();
		void nextWindow();
		void chooseLive();

		void setOnMomentChosen(std::function<void(std::optional<double>)> callback) {
			mOnMomentChosen = std::move(callback);
		}

		ui::Label caption{ "History" };
		ui::TransportButton back{ ui::TransportButton::Glyph::Previous };
		ui::Slider slider{ 0, 1, 1 };
		ui::TransportButton forward{ ui::TransportButton::Glyph::Next };
		ui::Label moment;
		ui::Dropdown precisionChoice;
		ui::Button live{ "Live" };

	private:
		void refresh();
		void refreshTicks();
		[[nodiscard]] bool hasHistory() const { return mFirst && mLast && *mLast > *mFirst; }
		[[nodiscard]] size_t lastWindowOffset() const;
		[[nodiscard]] double clampedToHistory(double moment) const;
		void keepMomentInWindow();

		std::optional<double> mFirst;
		std::optional<double> mLast;
		std::optional<double> mMoment;
		std::vector<double> mChanges;
		size_t mPrecision = 1;
		size_t mWindowOffset = 0;
		std::function<void(std::optional<double>)> mOnMomentChosen;
	};
}

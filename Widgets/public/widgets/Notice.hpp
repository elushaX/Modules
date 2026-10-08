#pragma once

#include "Animation.hpp"
#include "Label.hpp"

#include <string>

namespace tp::ui {

	class Notice : public TooltipBubble {
	public:
		static constexpr halnf kDefaultDurationMs = 2500;

		Notice();
		~Notice() override;

		void show(Widget& anchor, std::string text, halnf durationMs = kDefaultDurationMs);
		void hide();
		[[nodiscard]] bool isShown() const;
		[[nodiscard]] halnf remainingMs() const { return mRemainingMs; }

	protected:
		void onTick(halnf deltaMs) override;

	private:
		halnf mRemainingMs = 0;
	};
}

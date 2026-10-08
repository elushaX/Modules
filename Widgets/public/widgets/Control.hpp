#pragma once

#include "Animation.hpp"
#include "Painter.hpp"
#include "Widget.hpp"

#include <functional>
#include <vector>

namespace tp::ui {

	class Control : public Widget {
	public:
		Control();

		void setOnActivate(std::function<void()> onActivate) { mOnActivate = std::move(onActivate); }
		void setOnClicked(std::function<void(const Event&)> onClicked) { mOnClicked = std::move(onClicked); }
		void setOnLongPress(std::function<void()> onLongPress) { mOnLongPress = std::move(onLongPress); }
		[[nodiscard]] bool hasLongPress() const { return (bool) mOnLongPress; }

		void activate();

	protected:
		void onEvent(Event& event) override;
		void onTick(halnf deltaMs) override;
		void onHoverChanged() override;
		[[nodiscard]] bool acceptsKeyboardFocus() const override { return true; }

		[[nodiscard]] virtual RGBA backgroundTarget() const;
		void onThemeChanged() override;
		void onPressedChanged() override;
		void onOwnedPopupChanged() override;
		void onAttached() override;
		virtual void onActivated() {}

		[[nodiscard]] RGBA background() const { return mBackground.value(); }
		virtual void refreshVisualState();

		void animate(AnimatedFloat& value, halnf target);

	private:
		void runAnimations();

		AnimatedColor mBackground;
		std::vector<AnimatedFloat*> mRunningFloats;
		std::function<void()> mOnActivate;
		std::function<void(const Event&)> mOnClicked;
		std::function<void()> mOnLongPress;
	};
}

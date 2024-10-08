#pragma once

#include "Widget.hpp"

#include <functional>

namespace tp {
	class LabelWidget : public Widget {
	public:
		LabelWidget() {
			setDebug("label", { 0.1, 0.1, 0.1, 0.1 });
		}

		void setText(const std::string& text);

		[[nodiscard]] const std::string& getText() const;

		void draw(Canvas& canvas) override;

		[[nodiscard]] bool processesEvents() const override { return false; }

	private:
		std::string mText = "Text";

		halnf mPadding = 5;
		RGBA mColor = 1.f;
		halnf mSize = 17.f;
	};

	class ButtonWidget : public LabelWidget {
	public:
		ButtonWidget();

		void setAction(const std::function<void()>& action);

		void process(const EventHandler& eventHandler) override;
		void draw(Canvas& canvas) override;

		void mouseEnter() override;
		void mouseLeave() override;

		[[nodiscard]] bool processesEvents() const override { return true; }
		[[nodiscard]] bool needsNextFrame() const override;

		void endAnimations() override;
		void updateAnimations() override;

	private:
		std::function<void()> mAction;

		SpringRect mColorAnimated;

		halnf mRounding = 5;
		RGBA mColorHovered = { 0.0f, 0.4f, 0.4f, 1.f };
		RGBA mColor = { 0.0f, 0.0f, 0.0f, 1.f };
	};
}
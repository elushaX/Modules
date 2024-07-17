#pragma once

#include "Widget.hpp"

#include <functional>

namespace tp {
	class LabelWidget : public Widget {
	public:
		LabelWidget() = default;

		void setText(const std::string& text);
		[[nodiscard]] const std::string& getText() const;

		void draw(Canvas& canvas) override;

		[[nodiscard]] bool isPassThroughEvents() const override { return true; }

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

		[[nodiscard]] bool isPassThroughEvents() const override { return false; }

		[[nodiscard]] bool needUpdate() const override;

		void finishAnimations() override;

	private:
		std::function<void()> mAction;

		SpringRect mColorAnimated;

		halnf mRounding = 5;
		RGBA mColor = { 1.0f, 0.0f, 0.0f, 1.f };
		RGBA mColorHovered = { 0.0f, 0.0f, 0.0f, 1.f };
	};
}
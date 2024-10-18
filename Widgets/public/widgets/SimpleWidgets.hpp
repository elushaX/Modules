#pragma once

#include "Widget.hpp"

#include <functional>

namespace tp {
	class LabelWidget : public Widget {
	public:
		LabelWidget() : Widget() {
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

		void setColor(const RGBA& in);

	private:
		std::function<void()> mAction;

		SpringRect mColorAnimated;

		halnf mRounding = 5;
		RGBA mColorHovered = { 0.0f, 0.4f, 0.4f, 1.f };
		RGBA mColor = { 0.13f, 0.13f, 0.13f, 1.f };
	};

	class SliderWidget : public Widget {
		enum State {
			IDLE,
			HOVER,
			SLIDING,
		};

	public:
		SliderWidget() = default;

		void process(const EventHandler& eventHandler) override;
		void draw(Canvas& canvas) override;

		[[nodiscard]] bool processesEvents() const override { return true; }
		[[nodiscard]] bool needsNextFrame() const override { return mState != SLIDING; }

		[[nodiscard]] halnf val() const { return mFactor; }

	private:
		[[nodiscard]] RectF getHandleArea() const;

	private:
		halnf mFactor = 0;
		State mState = IDLE;

	private:
		halnf mHandleSize = 20;
		halnf mRounding = 5;

		RGBA mColorActive = { 0.5f, 0.5f, 0.5f, 1.f };
		RGBA mColorHovered = { 0.3f, 0.3f, 0.3f, 1.f };
		RGBA mColorIdle = { 0.2f, 0.2f, 0.2f, 1.f };
		RGBA mColorBG = { 0.08f, 0.08f, 0.08f, 1.0f };
	};
}
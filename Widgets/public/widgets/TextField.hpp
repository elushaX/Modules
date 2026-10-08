#pragma once

#include "Button.hpp"
#include "Control.hpp"
#include "TextEditBuffer.hpp"

#include <functional>
#include <string>

namespace tp::ui {

	class TextField : public Control {
	public:
		explicit TextField(std::string text = {});

		void setText(std::string text);
		[[nodiscard]] const std::string& text() const { return mBuffer.text(); }

		void setPlaceholder(std::string placeholder);
		void setFontSize(halnf fontSize);
		void setMaximumLength(size_t maximumLength) { mBuffer.setMaximumLength(maximumLength); }
		void setCharacterFilter(std::function<bool(const std::string&)> accepts) {
			mBuffer.setCharacterFilter(std::move(accepts));
		}

		[[nodiscard]] size_t caret() const { return mBuffer.caret(); }
		[[nodiscard]] size_t selectionAnchor() const { return mBuffer.anchor(); }
		[[nodiscard]] bool hasSelection() const { return mBuffer.hasSelection(); }
		[[nodiscard]] std::string selectedText() const { return mBuffer.selectedText(); }
		void selectAll();
		void setCaret(size_t position, bool extendSelection = false);

		void setOnEdited(std::function<void(const std::string&)> onEdited) { mOnEdited = std::move(onEdited); }
		void setOnEditingStarted(std::function<void()> callback) { mOnEditingStarted = std::move(callback); }
		void setOnCommitted(std::function<void(const std::string&)> onCommitted) { mOnCommitted = std::move(onCommitted); }

		[[nodiscard]] bool isEditing() const { return hasKeyboardFocus(); }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;
		void onTick(halnf deltaMs) override;
		void onKeyboardFocusChanged() override;
		void onDetached() override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		void handleKey(Event& event);
		void insert(const std::string& text);
		void commit();
		void edited();
		void updateTextInputArea();
		void restartBlink();

		[[nodiscard]] size_t positionAt(halnf localX) const;
		[[nodiscard]] halnf offsetOf(size_t position) const;
		[[nodiscard]] RectF textBox() const;
		[[nodiscard]] halnf fontSize() const;
		[[nodiscard]] halnf lineHeight() const;

		TextEditBuffer mBuffer;
		std::string mPlaceholder;
		halnf mFontSize = 0;
		halnf mScroll = 0;
		halnf mBlinkMs = 0;
		bool mDragging = false;
		bool mFocused = false;
		bool mFocusedByPress = false;
		std::function<void(const std::string&)> mOnEdited;
		std::function<void()> mOnEditingStarted;
		std::function<void(const std::string&)> mOnCommitted;
	};

	class StepButton : public Button {
	public:
		explicit StepButton(int direction);

		[[nodiscard]] int direction() const { return mDirection; }

	protected:
		[[nodiscard]] bool hasIcon() const override { return true; }
		void paintIcon(Painter& painter, const RectF& box) override;

	private:
		int mDirection;
	};

	class NumberField : public Widget {
	public:
		NumberField(halnf minimum, halnf maximum, halnf value, halnf step = 1);

		void setValue(halnf value);
		[[nodiscard]] halnf value() const { return mValue; }
		void setRange(halnf minimum, halnf maximum);
		void setStep(halnf step) { mStep = step; }
		void setDecimals(int decimals);
		void setSuffix(std::string suffix);

		void setOnChanged(std::function<void(halnf)> onChanged) { mOnChanged = std::move(onChanged); }

		[[nodiscard]] TextField& field() { return mField; }
		[[nodiscard]] StepButton& decrementButton() { return mDecrement; }
		[[nodiscard]] StepButton& incrementButton() { return mIncrement; }

		void stepBy(int steps);

	protected:
		void onEvent(Event& event) override;

	private:
		void apply(halnf value);
		void refreshText();
		[[nodiscard]] halnf clamped(halnf value) const;
		[[nodiscard]] std::string format(halnf value) const;

		halnf mMinimum;
		halnf mMaximum;
		halnf mValue;
		halnf mStep;
		int mDecimals = 0;
		std::string mSuffix;
		StepButton mDecrement{ -1 };
		TextField mField;
		StepButton mIncrement{ 1 };
		std::function<void(halnf)> mOnChanged;
	};
}

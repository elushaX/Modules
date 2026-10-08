#pragma once

#include "Control.hpp"
#include "Label.hpp"

#include <functional>
#include <vector>

namespace tp::ui {

	class Slider : public Control {
	public:
		enum class Scale { Linear, Logarithmic };

		explicit Slider(halnf minimum = 0, halnf maximum = 1, halnf value = 0);

		void setScale(Scale scale);
		[[nodiscard]] Scale scale() const { return mScale; }
		void setOrientation(Axis orientation);
		[[nodiscard]] Axis orientation() const { return mOrientation; }
		[[nodiscard]] halnf fractionForValue(halnf value) const;
		[[nodiscard]] halnf valueForFraction(halnf fraction) const;

		void setRange(halnf minimum, halnf maximum);
		void setValue(halnf value);
		void setStep(halnf step);

		[[nodiscard]] halnf value() const { return mValue; }
		[[nodiscard]] halnf minimum() const { return mMinimum; }
		[[nodiscard]] halnf maximum() const { return mMaximum; }
		[[nodiscard]] halnf fraction() const;
		[[nodiscard]] Vec2F windowPointForValue(halnf value) const;
		[[nodiscard]] bool isDragging() const { return mDragging; }

		void setOnChanged(std::function<void(halnf)> onChanged) { mOnChanged = std::move(onChanged); }
		void setOnValueShown(std::function<void()> onValueShown) { mOnValueShown = std::move(onValueShown); }
		void setOnDragging(std::function<void(bool)> onDragging) { mOnDragging = std::move(onDragging); }
		void setTrackGradient(std::function<std::vector<RGBA>()> stops) {
			mTrackGradient = std::move(stops);
			requestPaint();
		}
		[[nodiscard]] std::vector<RGBA> trackGradient() const {
			return mTrackGradient ? mTrackGradient() : std::vector<RGBA>();
		}
		void setTickMarks(std::vector<halnf> fractions);
		[[nodiscard]] const std::vector<halnf>& tickMarks() const { return mTickMarks; }
		[[nodiscard]] std::vector<halnf> visibleTickMarks() const;

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;
		void refreshVisualState() override;

	private:
		[[nodiscard]] RectF trackRect() const;
		[[nodiscard]] bool showsHandle() const;
		[[nodiscard]] halnf valueAt(Vec2F local) const;
		[[nodiscard]] bool isVertical() const { return mOrientation == Axis::Vertical; }
		[[nodiscard]] Vec2F pointOnTrack(const RectF& track, halnf amount) const;
		void paintTickMarks(Painter& painter, const RectF& track) const;
		[[nodiscard]] halnf snapped(halnf value) const;
		[[nodiscard]] bool isLogarithmic() const;
		void stepBy(halnf steps);
		void changeValue(halnf value);

		halnf mMinimum;
		halnf mMaximum;
		halnf mValue;
		halnf mStep = 0;
		Scale mScale = Scale::Linear;
		Axis mOrientation = Axis::Horizontal;
		bool mDragging = false;
		AnimatedFloat mHandleReveal{ 0 };
		std::function<void(halnf)> mOnChanged;
		std::function<void()> mOnValueShown;
		std::function<void(bool)> mOnDragging;
		std::function<std::vector<RGBA>()> mTrackGradient;
		std::vector<halnf> mTickMarks;
	};

	[[nodiscard]] std::vector<halnf> groupTickMarks(std::vector<halnf> fractions, halnf minimumGap);

	class LabeledSlider : public Widget {
	public:
		explicit LabeledSlider(std::string name, halnf minimum = 0, halnf maximum = 1, halnf value = 0);

		void setValueFormatter(std::function<std::string(halnf)> formatter);
		void setName(std::string name) { mName.setText(std::move(name)); }
		[[nodiscard]] const std::string& name() const { return mName.text(); }

		[[nodiscard]] Slider& slider() { return mSlider; }
		void setScale(Slider::Scale scale) { mSlider.setScale(scale); }
		[[nodiscard]] halnf value() const { return mSlider.value(); }
		void setValue(halnf value) { mSlider.setValue(value); }
		void setOnChanged(std::function<void(halnf)> onChanged);

	private:
		void refreshValueText();

		Widget mHeader;
		Label mName;
		Label mValueText;
		Slider mSlider;
		std::function<std::string(halnf)> mFormatter;
		std::function<void(halnf)> mOnChanged;
	};

	class ProgressBar : public Widget {
	public:
		ProgressBar();

		void setFraction(halnf fraction);
		[[nodiscard]] halnf fraction() const { return mFraction.target(); }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onTick(halnf deltaMs) override;
		void paint(Painter& painter) override;

	private:
		AnimatedFloat mFraction;
	};
}

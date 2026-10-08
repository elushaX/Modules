#pragma once

#include "RecordingPainter.hpp"
#include "Ui.hpp"
#include "Widget.hpp"

#include <string>

namespace tp::ui {
	class Slider;
}

namespace tp::ui::testing {

	class UiDriver {
	public:
		explicit UiDriver(Widget& root, Vec2F viewportSize = { 800, 600 });
		~UiDriver();

		UiDriver(const UiDriver&) = delete;
		UiDriver& operator=(const UiDriver&) = delete;

		[[nodiscard]] Ui& ui() { return mUi; }
		[[nodiscard]] size_t textMeasurements() const { return mTextMetrics.measurements(); }

		void setAutoSettle(bool autoSettle) { mAutoSettle = autoSettle; }
		void setPointerTool(PointerTool tool) { mSample.tool = tool; }
		void finishLayoutAnimations();
		void layout();

		void resize(Vec2F viewportSize);

		void moveTo(Vec2F windowPoint);
		void moveTo(const Widget& widget);
		void press(PointerButton button = PointerButton::Primary, Modifiers modifiers = {});
		void release(PointerButton button = PointerButton::Primary, Modifiers modifiers = {});
		void click(Vec2F windowPoint, Modifiers modifiers = {});
		void click(const Widget& widget, Modifiers modifiers = {});
		void drag(Vec2F from, Vec2F to, int steps = 4);
		void swipe(Vec2F from, Vec2F to, halnf durationMs, int steps = 8);
		void setSlider(Slider& slider, halnf value);
		void wheel(halnf amount);
		void horizontalWheel(halnf amount);
		void gesture(const TouchGesture& gesture);
		void key(InputID key, Modifiers modifiers = {});
		void holdKey(InputID key, int repeats, Modifiers modifiers = {});
		void pressKey(InputID key, Modifiers modifiers = {});
		void releaseKey(InputID key, Modifiers modifiers = {});
		void type(const std::string& text);

		void advance(halnf deltaMs);
		void settle(halnf maximumMs = 5000);
		void hoverFor(const Widget& widget, halnf durationMs);

		RecordingPainter& paint();

		[[nodiscard]] Vec2F centerOf(const Widget& widget);
		[[nodiscard]] std::string dump() const { return mUi.dumpTree(); }

	private:
		static constexpr halnf kFrameMs = 16;

		MonospaceTextMetrics mTextMetrics;
		Ui mUi;
		RecordingPainter mPainter;
		Vec2F mPointer = { -1000, -1000 };
		PointerSample mSample;
		double mClockMs = 1000;
		bool mAutoSettle = true;
	};
}

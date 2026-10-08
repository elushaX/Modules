#pragma once

#include "WidgetApplication.hpp"

#include <string>
#include <vector>

namespace tp::ui {

	class AppDriver {
	public:
		static constexpr halnf kFrameMs = 16;
		static constexpr halnf kClickGapMs = 600;
		static constexpr int kSettleFrameLimit = 120;

		explicit AppDriver(WidgetApplication& application);
		~AppDriver();

		AppDriver(const AppDriver&) = delete;
		AppDriver& operator=(const AppDriver&) = delete;

		void frame(halnf deltaMs = kFrameMs);
		void frames(int count);
		void advance(halnf milliseconds);

		void moveTo(Vec2F windowPoint);
		void press(InputID button = InputID::MOUSE1);
		void release(InputID button = InputID::MOUSE1);
		void click(Vec2F windowPoint, InputID button = InputID::MOUSE1);
		void click(const Widget& widget);
		void drag(Vec2F from, Vec2F to, int steps = 8);
		void scroll(Vec2F windowPoint, halnf amount);
		void penStroke(const std::vector<Vec2F>& points, const std::vector<PointerSample>& samples);

		void key(InputID key);
		void keyDown(InputID key);
		void keyUp(InputID key);
		void shortcut(InputID modifier, InputID key);
		void type(const std::string& utf8);

		void settle(const Widget& widget);

		[[nodiscard]] static Vec2F centerOf(const Widget& widget);
		[[nodiscard]] static bool isMoving(const Widget& widget);

	private:
		WidgetApplication& mApplication;
		WindowInputObserver& mInput;
	};
}

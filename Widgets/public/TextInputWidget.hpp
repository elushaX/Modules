#pragma once

#include "WidgetBase.hpp"

namespace tp {
	class TextInputWidget : public Widget {
	public:
		TextInputWidget();

		void eventDraw(Canvas&) override;

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override;

	public:
		enum { mMaxBufferSize = 512 };
		char mBuff[mMaxBufferSize] = "";
		bool nChanged = false;
		std::string mValue;
		std::string mId = "id";
		bool mMultiline = false;

		RGBA mAccentColor;
		RGBA mHoveredColor;
		RGBA mBaseColor;
		halnf mRounding = 0;
		halnf mPadding = 0;
	};
}
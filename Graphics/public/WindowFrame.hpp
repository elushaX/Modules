#pragma once

#include "Rect.hpp"
#include "WindowTypes.hpp"

namespace tp {

	class WindowFrame {
	public:
		static constexpr halnf kShadowMargin = 18;
		static constexpr halnf kCornerRadius = 12;
		static constexpr halnf kResizeBorder = 10;
		static constexpr halnf kMinimumInnerResizeBorder = 3;
		static constexpr halnf kCornerResizeLength = 18;

		void setContentSize(Vec2F size) { mContentSize = size; }
		void setTiled(bool tiled) { mTiled = tiled; }
		void setTranslucent(bool translucent) { mTranslucent = translucent; }

		[[nodiscard]] Vec2F contentSize() const { return mContentSize; }
		[[nodiscard]] bool isTiled() const { return mTiled; }
		[[nodiscard]] bool isTranslucent() const { return mTranslucent; }

		[[nodiscard]] bool hasShadow() const { return mTranslucent && !mTiled; }
		[[nodiscard]] halnf margin() const { return hasShadow() ? kShadowMargin : 0; }
		[[nodiscard]] halnf cornerRadius() const { return hasShadow() ? kCornerRadius : 0; }

		[[nodiscard]] Vec2F surfaceSize() const;
		[[nodiscard]] RectF contentRect() const;
		[[nodiscard]] RectF inputRect() const;

		[[nodiscard]] WindowEdge edgeAt(Vec2F surfacePoint) const;

	private:
		[[nodiscard]] halnf outerResizeBorder() const;
		[[nodiscard]] halnf innerResizeBorder() const;

		Vec2F mContentSize = { 0, 0 };
		bool mTiled = false;
		bool mTranslucent = true;
	};
}

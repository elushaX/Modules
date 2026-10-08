#pragma once

#include "Animation.hpp"
#include "Widget.hpp"

#include <functional>

namespace tp::ui {

	class ResizeHandle : public Widget {
	public:
		explicit ResizeHandle(Axis dragAxis);

		void setOnDragStarted(std::function<void()> onStarted) { mOnStarted = std::move(onStarted); }
		void setOnDragged(std::function<void(halnf windowCoordinate)> onDragged) { mOnDragged = std::move(onDragged); }
		void setOnDragFinished(std::function<void()> onFinished) { mOnFinished = std::move(onFinished); }

		[[nodiscard]] Axis dragAxis() const { return mDragAxis; }
		[[nodiscard]] bool isDragging() const { return mDragging; }
		[[nodiscard]] halnf revealAmount() const { return mReveal.value(); }

		[[nodiscard]] RectF lineRect() const;

	protected:
		void onEvent(Event& event) override;
		void onHoverChanged() override;
		void onTick(halnf deltaMs) override;
		void paint(Painter& painter) override;

	private:
		void refreshReveal();

		Axis mDragAxis;
		bool mDragging = false;
		halnf mGrabOffset = 0;
		AnimatedFloat mReveal{ 0, Motion::quick() };

		std::function<void()> mOnStarted;
		std::function<void(halnf)> mOnDragged;
		std::function<void()> mOnFinished;
	};
}

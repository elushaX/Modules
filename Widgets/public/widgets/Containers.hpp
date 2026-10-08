#pragma once

#include <functional>
#include <vector>
#include "ScrollPhysics.hpp"
#include "Animation.hpp"
#include "Painter.hpp"
#include "Widget.hpp"

namespace tp::ui {

	class Panel : public Widget {
	public:
		enum class Surface { None, Window, Base, Raised };

		explicit Panel(Surface surface = Surface::Base);

		void setSurface(Surface surface);
		void setCornerRadius(halnf radius);
		void setBorder(bool border);
		[[nodiscard]] halnf effectiveCornerRadius() const;

	protected:
		void paint(Painter& painter) override;

	private:
		Surface mSurface;
		halnf mCornerRadius = -1;
		bool mBorder = false;
	};

	class Separator : public Widget {
	public:
		explicit Separator(Axis lineAxis = Axis::Horizontal);

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		[[nodiscard]] bool hitTest(Vec2F) const override { return false; }

	private:
		Axis mLineAxis;
	};

	class Spacer : public Widget {
	public:
		Spacer();
		explicit Spacer(halnf length);

	protected:
		[[nodiscard]] bool hitTest(Vec2F) const override { return false; }
	};

	class ScrollView;

	class ScrollBar : public Widget {
	public:
		explicit ScrollBar(ScrollView& view);

		[[nodiscard]] halnf revealAmount() const { return mReveal.value(); }
		[[nodiscard]] bool isDragging() const { return mDragging; }
		[[nodiscard]] RectF trackRect() const;
		[[nodiscard]] RectF thumbRect() const;

		void flash(halnf durationMs);

	protected:
		void onEvent(Event& event) override;
		void onHoverChanged() override;
		void onTick(halnf deltaMs) override;
		void paint(Painter& painter) override;

	private:
		[[nodiscard]] halnf offsetForThumbAt(halnf pointerAlong) const;
		void refreshReveal();

		ScrollView& mView;
		bool mDragging = false;
		halnf mGrabOffset = 0;
		halnf mFlashMs = 0;
		AnimatedFloat mReveal{ 0, Motion::fluid() };
	};

	class ScrollView : public Widget {
	public:
		struct DragSample {
			uint4 timeMs;
			halnf offset;
		};

		explicit ScrollView(Axis scrollAxis = Axis::Vertical);

		void setContent(Widget* content);
		[[nodiscard]] Widget* content() const { return mContent; }

		void scrollTo(halnf offset);
		void scrollBy(halnf delta);
		[[nodiscard]] halnf offset() const { return mOffset.target(); }
		[[nodiscard]] halnf maximumOffset() const;
		[[nodiscard]] halnf contentLength() const { return mContentLength; }
		[[nodiscard]] bool isScrollable() const { return maximumOffset() > 0; }

		void setWheelStep(halnf step) { mWheelStep = step; }
		void setOnScrolled(std::function<void(halnf offset)> callback) { mOnScrolled = std::move(callback); }
		void setShowsScrollBar(bool shows) {
			mShowsScrollBar = shows;
			requestLayout();
		}

		[[nodiscard]] Axis scrollAxis() const { return mScrollAxis; }
		[[nodiscard]] halnf viewportLength() const;
		[[nodiscard]] halnf currentOffset() const { return mOffset.value(); }
		[[nodiscard]] bool isBouncing() const { return mStartStretch.isActive() || mEndStretch.isActive(); }
		[[nodiscard]] halnf stretchOffset() const;
		[[nodiscard]] const StretchEffect& startStretch() const { return mStartStretch; }
		[[nodiscard]] const StretchEffect& endStretch() const { return mEndStretch; }
		[[nodiscard]] const FlingSpline& fling() const { return mFling; }
		void jumpTo(halnf offset);

		[[nodiscard]] ScrollBar& scrollBar() { return mScrollBar; }
		[[nodiscard]] halnf scrollBarReveal() const { return mScrollBar.revealAmount(); }
		[[nodiscard]] halnf flingVelocity() const { return mFlingVelocity; }
		[[nodiscard]] static halnf averageVelocity(const std::vector<DragSample>& samples, uint4 nowMs);

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onEvent(Event& event) override;
		void filterDescendantEvent(Event& event) override;
		void onTick(halnf deltaMs) override;

	private:
		class ScrollLayout;
		friend class ScrollBar;

		static constexpr halnf kDragThreshold = 8;
		static constexpr halnf kFlingMinimumSpeed = 0.1f;
		static constexpr halnf kFlingRecentMs = 100;

		void keepOffsetInRange();
		void applyOffset();
		void trackDrag(Event& event);
		void dragBy(halnf delta);
		void rememberDragSample(uint4 timeMs);
		[[nodiscard]] halnf averageDragVelocity(uint4 nowMs) const;
		[[nodiscard]] bool startsDrag(const Event& event) const;

		Axis mScrollAxis;
		Widget* mContent = nullptr;
		ScrollBar mScrollBar{ *this };
		AnimatedFloat mOffset;
		halnf mContentLength = 0;
		halnf mWheelStep = 48;
		std::function<void(halnf)> mOnScrolled;
		halnf mReportedOffset = 0;
		bool mShowsScrollBar = true;
		bool mDragPending = false;
		bool mDragging = false;
		Vec2F mDragStart = { 0, 0 };
		halnf mDragStartOffset = 0;
		std::vector<DragSample> mDragSamples;
		halnf mFlingVelocity = 0;
		halnf mLastDragAlong = 0;
		FlingSpline mFling;
		StretchEffect mStartStretch;
		StretchEffect mEndStretch;
	};
}

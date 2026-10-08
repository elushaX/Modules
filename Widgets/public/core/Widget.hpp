#pragma once

#include "Sizing.hpp"
#include "WindowTypes.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tp::ui {

	class Ui;
	class Layout;
	class Painter;
	class Event;
	class AnimatedRect;
	struct Theme;
	struct TextStyle;
	class TextMetrics;

	class Widget {
	public:
		Widget();
		virtual ~Widget();

		Widget(const Widget&) = delete;
		Widget& operator=(const Widget&) = delete;

		void addChild(Widget& child);
		void insertChild(size_t index, Widget& child);
		void removeChild(Widget& child);
		void removeAllChildren();
		void bringToFront();

		[[nodiscard]] Widget* parent() const { return mParent; }
		[[nodiscard]] const std::vector<Widget*>& children() const { return mChildren; }
		[[nodiscard]] bool isSelfOrAncestorOf(const Widget& other) const;

		void setSizing(const Sizing& sizing);
		void setSizing(const AxisSizing& horizontal, const AxisSizing& vertical);
		void setSizing(Axis axis, const AxisSizing& sizing);
		[[nodiscard]] const Sizing& sizing() const { return mSizing; }

		void setLayout(std::unique_ptr<Layout> layout);
		[[nodiscard]] Layout* layout() const { return mLayout.get(); }

		template <typename LayoutType, typename... Arguments>
		LayoutType& emplaceLayout(Arguments&&... arguments) {
			auto layout = std::make_unique<LayoutType>(std::forward<Arguments>(arguments)...);
			LayoutType& reference = *layout;
			setLayout(std::move(layout));
			return reference;
		}

		void requestPosition(Vec2F position);
		[[nodiscard]] Vec2F requestedPosition() const { return mRequestedPosition; }

		[[nodiscard]] const RectF& rect() const { return mRect; }
		[[nodiscard]] const RectF& targetRect() const { return mTargetRect; }
		[[nodiscard]] Vec2F size() const { return mRect.size; }
		[[nodiscard]] RectF localRect() const { return makeRect({ 0, 0 }, mRect.size); }
		[[nodiscard]] Vec2F windowPosition() const;
		[[nodiscard]] RectF windowRect() const { return makeRect(windowPosition(), mRect.size); }
		[[nodiscard]] Vec2F toLocal(Vec2F windowPoint) const { return windowPoint - windowPosition(); }

		void setGeometryAnimated(bool animated);
		[[nodiscard]] bool isGeometryAnimated() const { return mGeometryAnimated; }
		[[nodiscard]] bool isGeometryMoving() const;
		void animateGeometryFrom(const RectF& start);
		void appearFrom(const RectF& start);
		void finishGeometryAnimation();
		[[nodiscard]] virtual bool isTransitioning() const { return false; }

		void setClipsChildren(bool clips) { mClipsChildren = clips; }
		[[nodiscard]] bool clipsChildren() const { return mClipsChildren; }

		void setVisible(bool visible);
		[[nodiscard]] bool isVisible() const { return mVisible; }

		void setEnabled(bool enabled);
		[[nodiscard]] bool isEnabled() const { return mEnabled; }
		[[nodiscard]] bool isEffectivelyEnabled() const;

		[[nodiscard]] bool isHovered() const { return mHovered; }
		[[nodiscard]] bool isPressed() const { return mPressed; }
		[[nodiscard]] bool hasKeyboardFocus() const;
		[[nodiscard]] bool showsFocusRing() const;
		[[nodiscard]] bool containsActiveWidget() const { return mContainsActive; }

		void setCursor(CursorShape cursor);
		[[nodiscard]] CursorShape cursor() const { return mCursor; }

		void setTooltip(std::string text) { mTooltip = std::move(text); }
		[[nodiscard]] const std::string& tooltip() const { return mTooltip; }

		void setDebugName(std::string name) { mDebugName = std::move(name); }
		[[nodiscard]] const std::string& debugName() const { return mDebugName; }

		void requestLayout();
		void requestPaint();

		[[nodiscard]] Ui* ui() const { return mUi; }
		[[nodiscard]] const Theme& theme() const;
		[[nodiscard]] halnf textWidth(const std::string& text, halnf fontSize) const;
		[[nodiscard]] halnf textWidth(const std::string& text, const TextStyle& style) const;
		[[nodiscard]] std::vector<halnf> prefixWidths(const std::string& text, const TextStyle& style) const;
		[[nodiscard]] const TextMetrics& textMetrics() const;

	protected:
		[[nodiscard]] virtual Extent measureContent(Axis axis, halnf crossLength) const;
		virtual void onEvent(Event& event);
		virtual void filterDescendantEvent(Event&) {}
		[[nodiscard]] virtual const Widget* raisedChild() const { return nullptr; }
		[[nodiscard]] virtual Widget* defaultKeyTarget() { return this; }
		virtual void paintRaisedBackdrop(Painter&, const RectF&) {}
		virtual void onTick(halnf deltaMs);
		virtual void finishTransitions() {}
		virtual void paint(Painter& painter);
		virtual void paintOverlay(Painter& painter);
		[[nodiscard]] virtual bool hitTest(Vec2F localPoint) const;
		[[nodiscard]] virtual bool acceptsKeyboardFocus() const { return false; }
		virtual void onHoverChanged() {}
		virtual void onPressedChanged() {}
		virtual void onOwnedPopupChanged() {}
		virtual void onActiveWithinChanged() {}
		virtual void onKeyboardFocusChanged() {}
		virtual void onThemeChanged() {}
		virtual void onInteractionModeChanged() {}
		virtual void onWindowHostChanged() {}
		virtual void onAttached() {}
		virtual void onDetached() {}

		void startTicking();
		void stopTicking();

	private:
		friend class WidgetInternals;

		struct MeasureCacheEntry {
			bool valid = false;
			halnf crossLength = 0;
			Extent extent;
		};

		void attachSubtree(Ui& ui);
		void detachSubtree();

		Widget* mParent = nullptr;
		std::vector<Widget*> mChildren;
		Ui* mUi = nullptr;

		Sizing mSizing;
		std::unique_ptr<Layout> mLayout;
		Vec2F mRequestedPosition = { 0, 0 };

		RectF mRect = makeRect(0, 0, 0, 0);
		RectF mTargetRect = makeRect(0, 0, 0, 0);
		Vec2F mArrangedSize = { -1, -1 };
		RectF mAppearFrom = makeRect(0, 0, -1, -1);
		bool mPlaced = false;
		bool mAppearing = false;
		bool mGeometryAnimated = true;
		bool mNeedsLayout = true;
		MeasureCacheEntry mMeasureCache[2];
		std::unique_ptr<AnimatedRect> mGeometry;

		bool mVisible = true;
		bool mClipsChildren = false;
		bool mEnabled = true;
		bool mHovered = false;
		bool mPressed = false;
		bool mContainsActive = false;
		bool mTicking = false;
		CursorShape mCursor = CursorShape::Default;

		std::string mTooltip;
		std::string mDebugName;
	};
}

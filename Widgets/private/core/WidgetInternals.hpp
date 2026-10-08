#pragma once

#include "Widget.hpp"
#include "Animation.hpp"
#include "Ui.hpp"

namespace tp::ui {

	class WidgetInternals {
	public:
		static void attach(Widget& widget, Ui& ui) { widget.attachSubtree(ui); }
		static void detach(Widget& widget) { widget.detachSubtree(); }

		static Extent measureContent(const Widget& widget, Axis axis, halnf crossLength) {
			return widget.measureContent(axis, crossLength);
		}

		static void deliver(Widget& widget, Event& event) { widget.onEvent(event); }
		static void filter(Widget& widget, Event& event) { widget.filterDescendantEvent(event); }
		static const Widget* raisedChild(const Widget& widget) { return widget.raisedChild(); }
		static Widget* defaultKeyTarget(Widget& widget) { return widget.defaultKeyTarget(); }
		static void paintRaisedBackdrop(Widget& widget, Painter& painter, const RectF& childRect) {
			widget.paintRaisedBackdrop(painter, childRect);
		}
		static void tick(Widget& widget, halnf deltaMs) { widget.onTick(deltaMs); }
		static void paint(Widget& widget, Painter& painter) { widget.paint(painter); }
		static void paintOverlay(Widget& widget, Painter& painter) { widget.paintOverlay(painter); }
		static bool hitTest(const Widget& widget, Vec2F localPoint) { return widget.hitTest(localPoint); }
		static bool acceptsKeyboardFocus(const Widget& widget) { return widget.acceptsKeyboardFocus(); }

		static void setHovered(Widget& widget, bool hovered) {
			if (widget.mHovered == hovered) return;
			widget.mHovered = hovered;
			widget.onHoverChanged();
			widget.requestPaint();
		}

		static void notifyOwnedPopupChanged(Widget& owner) {
			owner.onOwnedPopupChanged();
			owner.requestPaint();
		}

		static void setPressed(Widget& widget, bool pressed) {
			if (widget.mPressed == pressed) return;
			widget.mPressed = pressed;
			widget.onPressedChanged();
			widget.requestPaint();
		}

		static void setContainsActive(Widget& widget, bool containsActive) {
			if (widget.mContainsActive == containsActive) return;
			widget.mContainsActive = containsActive;
			widget.onActiveWithinChanged();
			widget.requestPaint();
		}

		static void notifyKeyboardFocusChanged(Widget& widget) {
			widget.onKeyboardFocusChanged();
			widget.requestPaint();
		}

		static void notifyThemeChanged(Widget& widget) {
			widget.onThemeChanged();
			for (Widget* child : widget.mChildren)
				notifyThemeChanged(*child);
		}

		static void notifyInteractionModeChanged(Widget& widget) {
			widget.onInteractionModeChanged();
			for (Widget* child : widget.mChildren)
				notifyInteractionModeChanged(*child);
		}

		static void notifyWindowHostChanged(Widget& widget) {
			widget.onWindowHostChanged();
			for (Widget* child : widget.mChildren)
				notifyWindowHostChanged(*child);
		}

		static bool needsLayout(const Widget& widget) {
			return widget.mNeedsLayout || !(widget.mArrangedSize == widget.mRect.size);
		}
		static void markLaidOut(Widget& widget) {
			widget.mNeedsLayout = false;
			widget.mArrangedSize = widget.mRect.size;
		}

		static bool assignTarget(Widget& widget, const RectF& target) {
			const RectF previousRect = widget.mRect;
			const bool firstPlacement = !widget.mPlaced;
			const bool targetChanged = !(widget.mTargetRect == target);

			widget.mTargetRect = target;
			widget.mPlaced = true;

			if (firstPlacement && widget.mAppearing && widget.mGeometryAnimated && widget.mUi) {
				widget.mAppearing = false;
				widget.mRect = widget.mAppearFrom.size.x < 0 ? collapsedAt(target) : widget.mAppearFrom;
				widget.mAppearFrom = makeRect(0, 0, -1, -1);
				startGeometry(widget, target);
				return true;
			}
			widget.mAppearing = false;

			const bool tracksViewport = widget.mUi && widget.mUi->isViewportResizing();
			if (
				firstPlacement || !widget.mGeometryAnimated || !widget.mUi || tracksViewport || followsMovingAncestor(widget)
			) {
				if (widget.mGeometry) widget.mGeometry->jumpTo(target);
				widget.mRect = target;
				return !(previousRect == widget.mRect);
			}

			if (targetChanged) startGeometry(widget, target);
			return false;
		}

		static RectF collapsedAt(const RectF& target) { return makeRect(target.center(), Vec2F(0.0f, 0.0f)); }

		static bool followsMovingAncestor(const Widget& widget) {
			for (const Widget* current = &widget; current->mParent; current = current->mParent) {
				const Widget* parent = current->mParent;
				if (parent->isGeometryMoving() || parent->isTransitioning()) return true;
				for (const Widget* sibling : parent->mChildren) {
					if (sibling == current) break;
					if (sibling->isTransitioning()) return true;
				}
			}
			return false;
		}

		static void startGeometry(Widget& widget, const RectF& target) {
			if (!widget.mGeometry) {
				widget.mGeometry =
					std::make_unique<AnimatedRect>(widget.mRect, Motion::fluid(), AnimatedRect::kGeometryTolerance);
			} else if (!widget.mGeometry->isAnimating()) {
				widget.mGeometry->jumpTo(widget.mRect);
			}
			widget.mGeometry->animateTo(target);
			widget.mUi->startGeometryAnimation(widget);
		}

		static void resetPlacement(Widget& widget) {
			widget.mPlaced = false;
			for (Widget* child : widget.mChildren)
				resetPlacement(*child);
		}

		static bool stepGeometry(Widget& widget, halnf deltaMs) {
			if (!widget.mGeometry) return false;

			const bool moving = widget.mGeometry->advance(deltaMs);
			const RectF current = moving ? widget.mGeometry->value() : widget.mTargetRect;

			if (!(current == widget.mRect)) {
				widget.mRect = current;
				markArrangeNeeded(widget);
			}
			return moving;
		}

		static void markArrangeNeeded(Widget& widget) {
			for (Widget* iterator = &widget; iterator; iterator = iterator->mParent)
				iterator->mNeedsLayout = true;
			if (widget.mUi) widget.mUi->scheduleLayout();
		}

		static bool cachedMeasure(const Widget& widget, Axis axis, halnf crossLength, Extent& extent) {
			const auto& entry = widget.mMeasureCache[(int) axis];
			if (!entry.valid || entry.crossLength != crossLength) return false;
			extent = entry.extent;
			return true;
		}

		static void storeMeasure(Widget& widget, Axis axis, halnf crossLength, const Extent& extent) {
			widget.mMeasureCache[(int) axis] = { true, crossLength, extent };
		}
	};
}

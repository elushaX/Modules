#pragma once

#include "Painter.hpp"
#include "Widget.hpp"

#include <functional>
#include <vector>

namespace tp::ui {

	class ReorderableList : public Widget {
	public:
		enum class DragMode { Grip, LongPress, None };

		static constexpr halnf kGripWidth = 14;
		static constexpr halnf kDragThreshold = 5;

		explicit ReorderableList(halnf gap = 2);

		void addItem(Widget& item);
		void clearItems();
		[[nodiscard]] size_t itemCount() const { return mItems.size(); }
		[[nodiscard]] Widget& item(size_t index) const { return *mItems[index]; }

		void setOnMoved(std::function<void(size_t from, size_t to)> onMoved) { mOnMoved = std::move(onMoved); }
		void setOnDroppedOnto(std::function<void(size_t dragged, size_t target)> onDroppedOnto) {
			mOnDroppedOnto = std::move(onDroppedOnto);
		}
		void setOnHeld(std::function<void(size_t index)> onHeld) { mOnHeld = std::move(onHeld); }
		void setCanAcceptDrop(std::function<bool(size_t dragged, size_t target)> canAcceptDrop) {
			mCanAcceptDrop = std::move(canAcceptDrop);
		}
		void setDragMode(DragMode mode);
		[[nodiscard]] DragMode dragMode() const { return mDragMode; }
		[[nodiscard]] long dropTarget() const { return mDragging ? mDropTarget : -1; }

		[[nodiscard]] bool isDragging() const { return mDragging; }
		[[nodiscard]] long draggedIndex() const { return mDragging ? (long) mDragged : -1; }
		[[nodiscard]] long dropIndex() const { return mDragging ? (long) mDrop : -1; }
		[[nodiscard]] RectF gripRect(size_t index) const;

		void cancelDrag();

	protected:
		void onEvent(Event& event) override;
		void onHoverChanged() override;
		void filterDescendantEvent(Event& event) override;
		void paint(Painter& painter) override;
		[[nodiscard]] const Widget* raisedChild() const override;
		void paintRaisedBackdrop(Painter& painter, const RectF& childRect) override;
		[[nodiscard]] bool hitTest(Vec2F localPoint) const override;
		[[nodiscard]] bool acceptsKeyboardFocus() const override { return mDragging; }

	private:
		class ListLayout;
		friend class ListLayout;

		void handlePointer(Event& event);
		void updateGripHover(Vec2F localPoint);
		void paintGrip(Painter& painter, const RectF& grip, const RGBA& color) const;
		[[nodiscard]] RectF gripRectFor(const RectF& row) const;
		[[nodiscard]] bool isOnGrip(Vec2F localPoint, size_t index) const;
		[[nodiscard]] bool hasNestedListAround(const Widget* target) const;
		void beginDrag(Vec2F windowPoint);
		void updateDrag(Vec2F windowPoint);
		void finishDrag();
		void endDragging();
		[[nodiscard]] long itemIndexAt(Vec2F localPoint) const;
		[[nodiscard]] size_t slotFor(halnf draggedTop, halnf draggedBottom) const;
		[[nodiscard]] halnf itemHeight(size_t index) const;
		[[nodiscard]] long dropTargetAt(halnf pointerY) const;
		[[nodiscard]] bool usesGrips() const { return mDragMode == DragMode::Grip; }
		[[nodiscard]] halnf gripWidth() const { return usesGrips() ? kGripWidth : 0; }

		std::vector<Widget*> mItems;
		halnf mGap;
		std::function<void(size_t, size_t)> mOnMoved;
		std::function<void(size_t, size_t)> mOnDroppedOnto;
		std::function<bool(size_t, size_t)> mCanAcceptDrop;
		std::function<void(size_t)> mOnHeld;
		bool mHeld = false;
		DragMode mDragMode = DragMode::Grip;
		long mDropTarget = -1;

		bool mPressed = false;
		bool mDragging = false;
		bool mPressedOnGrip = false;
		size_t mPressedIndex = 0;
		size_t mDragged = 0;
		size_t mDrop = 0;
		Vec2F mPressPoint = { 0, 0 };
		halnf mGrabOffset = 0;
		halnf mPressLocalY = 0;
		halnf mListHeight = 0;
		Widget* mFocusBeforeDrag = nullptr;
		halnf mDragY = 0;
		long mHoveredGrip = -1;
	};
}

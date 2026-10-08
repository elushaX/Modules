#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "FullViewFrame.hpp"
#include "InputCodes.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "Widget.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tp::ui {

	using KanbanItemId = uint64_t;

	enum class KanbanOrdering { Manual, ModelDefined };

	struct KanbanColumn {
		std::string title;
		std::optional<RGBA> accent;
		std::optional<size_t> cardLimit;
		bool acceptsDrops = true;
		KanbanOrdering ordering = KanbanOrdering::Manual;
		bool visible = true;
	};

	struct KanbanMove {
		KanbanItemId item = 0;
		size_t fromColumn = 0;
		size_t fromIndex = 0;
		size_t toColumn = 0;
		size_t toIndex = 0;

		bool operator==(const KanbanMove&) const = default;
	};

	struct KanbanDropTarget {
		size_t column = 0;
		size_t index = 0;
		bool accepted = false;
	};

	class KanbanModel {
	public:
		virtual ~KanbanModel() = default;

		[[nodiscard]] virtual std::vector<KanbanItemId> items(size_t column) const = 0;
		[[nodiscard]] virtual bool canMove(const KanbanMove&) const { return true; }
	};

	class KanbanCard : public Widget {
	public:
		explicit KanbanCard(KanbanItemId id);

		[[nodiscard]] KanbanItemId id() const { return mId; }

		void setTitle(std::string title) { mTitle.setText(std::move(title)); }
		void setDetail(std::string detail);
		void setFixedHeight(std::optional<halnf> height);
		void setAccent(std::optional<RGBA> accent);
		void setBadge(std::optional<RGBA> color);
		void setContent(Widget& content);
		[[nodiscard]] Widget* content() const { return mContent; }

		[[nodiscard]] const std::string& title() const { return mTitle.text(); }
		[[nodiscard]] const std::string& detail() const { return mDetail.text(); }
		[[nodiscard]] const std::optional<RGBA>& accent() const { return mAccent; }
		[[nodiscard]] bool isSelected() const { return mSelected; }

	protected:
		void paint(Painter& painter) override;
		void paintOverlay(Painter& painter) override;
		void onHoverChanged() override { requestPaint(); }

	private:
		friend class KanbanBoard;

		void setSelected(bool selected);
		[[nodiscard]] size_t detailLinesFor(halnf height) const;

		KanbanItemId mId;
		Label mTitle;
		Label mDetail;
		std::optional<RGBA> mAccent;
		std::optional<RGBA> mBadge;
		Widget* mContent = nullptr;
		bool mSelected = false;
	};

	class KanbanCardStack : public Widget {
	public:
		KanbanCardStack();

		[[nodiscard]] const std::vector<KanbanCard*>& cards() const { return mCards; }
		[[nodiscard]] std::optional<size_t> placeholderIndex() const { return mPlaceholderIndex; }

	protected:
		void paint(Painter& painter) override;

	private:
		friend class KanbanBoard;
		class CardLayout;

		void setCards(const std::vector<KanbanCard*>& cards, const std::unordered_map<KanbanCard*, RectF>& landings);
		void removeCard(KanbanCard& card);
		void setPlaceholder(std::optional<size_t> index, halnf height);
		[[nodiscard]] size_t insertionIndexAt(halnf windowY) const;
		[[nodiscard]] std::optional<size_t> indexOf(const KanbanCard& card) const;

		std::vector<KanbanCard*> mCards;
		std::unordered_map<KanbanCard*, RectF> mLandings;
		std::optional<size_t> mPlaceholderIndex;
		halnf mPlaceholderHeight = 0;
		RectF mPlaceholderRect = makeRect(0, 0, 0, 0);
	};

	class KanbanColumnView : public Widget {
	public:
		explicit KanbanColumnView(const KanbanColumn& column);

		[[nodiscard]] Label& title() { return mTitle; }
		[[nodiscard]] Label& count() { return mCount; }
		[[nodiscard]] Widget& header() { return mHeader; }
		[[nodiscard]] const Widget& header() const { return mHeader; }
		[[nodiscard]] GlyphButton& optionsButton() { return mOptions; }
		[[nodiscard]] Menu& optionsMenu() { return mOptionsMenu; }
		[[nodiscard]] ScrollView& scroller() { return mScroller; }
		[[nodiscard]] KanbanCardStack& stack() { return mStack; }

	protected:
		void paint(Painter& painter) override;

	private:
		friend class KanbanBoard;

		enum class DropState { None, Accepted, Refused };

		void setDropState(DropState state);
		void refreshCount();

		KanbanColumn mColumn;
		Widget mHeader;
		Label mTitle;
		Label mCount;
		GlyphButton mOptions{ GlyphButton::Glyph::Options };
		Menu mOptionsMenu;
		ScrollView mScroller{ Axis::Vertical };
		KanbanCardStack mStack;
		DropState mDropState = DropState::None;
	};

	// Columns of cards that can be dragged between columns. Dragging a column header sideways moves the whole column.
	// Every column header has an options menu that shows or hides any column; at least one column stays visible.
	// Double-clicking a card opens its full view: fullViewFrame() grows out of the card to cover the board while the
	// columns fade behind it, and holds the most detailed widget the host gives it. Back, or Escape while the frame
	// itself has keyboard focus, shrinks it back into the card.
	class KanbanBoard : public Widget {
	public:
		using CardFactory = std::function<std::unique_ptr<KanbanCard>(KanbanItemId)>;
		using CardUpdater = std::function<void(KanbanCard&)>;

		static constexpr halnf kDragThreshold = 5;
		static constexpr halnf kColumnWheelStep = 60;

		explicit KanbanBoard(const KanbanModel& model);
		~KanbanBoard() override;

		void setColumns(std::vector<KanbanColumn> columns);
		[[nodiscard]] const std::vector<KanbanColumn>& columns() const { return mColumns; }
		void setColumnVisible(size_t column, bool visible);
		[[nodiscard]] bool isColumnVisible(size_t column) const { return mColumns[column].visible; }
		[[nodiscard]] size_t visibleColumnCount() const;
		void setOnColumnVisibilityChanged(std::function<void(size_t column, bool visible)> callback) {
			mOnColumnVisibilityChanged = std::move(callback);
		}
		void moveColumn(size_t from, size_t to);
		void setOnColumnMoved(std::function<void(size_t from, size_t to)> callback) {
			mOnColumnMoved = std::move(callback);
		}
		void setColumnWidthRange(halnf minimum, halnf maximum);
		void setCardHeight(std::optional<halnf> height);
		[[nodiscard]] std::optional<halnf> cardHeight() const { return mCardHeight; }

		void setCardFactory(CardFactory factory) { mCardFactory = std::move(factory); }
		void setCardUpdater(CardUpdater updater) { mCardUpdater = std::move(updater); }

		void modelChanged();
		void refreshCards();

		void select(std::optional<KanbanItemId> id);
		[[nodiscard]] std::optional<KanbanItemId> selection() const { return mSelection; }

		void setOnSelected(std::function<void(KanbanItemId)> callback) { mOnSelected = std::move(callback); }
		void setOnActivated(std::function<void(KanbanItemId)> callback) { mOnActivated = std::move(callback); }
		void setOnMoved(std::function<void(const KanbanMove&)> callback) { mOnMoved = std::move(callback); }
		void setOnDeleteRequested(std::function<void(KanbanItemId)> callback) { mOnDeleteRequested = std::move(callback); }
		void setOnFullViewChanged(std::function<void(std::optional<KanbanItemId>)> callback) {
			mOnFullViewChanged = std::move(callback);
		}

		void enterFullView(KanbanItemId id, bool animated = true);
		void leaveFullView(bool animated = true);
		[[nodiscard]] std::optional<KanbanItemId> fullViewItem() const { return mFull; }
		[[nodiscard]] halnf fullViewAmount() const { return mFrame.progress(); }
		[[nodiscard]] FullViewFrame& fullViewFrame() { return mFrame; }

		[[nodiscard]] bool isDragging() const { return mDrag.has_value(); }
		[[nodiscard]] std::optional<KanbanItemId> draggedItem() const;
		[[nodiscard]] std::optional<KanbanDropTarget> dropTarget() const;
		void cancelDrag();

		[[nodiscard]] bool isMovingColumn() const { return mColumnDrag.has_value(); }
		[[nodiscard]] std::optional<size_t> columnDropIndex() const;

		[[nodiscard]] size_t columnCount() const { return mColumnViews.size(); }
		[[nodiscard]] KanbanColumnView& columnView(size_t column) { return *mColumnViews[column]; }
		[[nodiscard]] ScrollView& columnScroller() { return mColumnScroller; }
		[[nodiscard]] KanbanCard* card(KanbanItemId id) const;
		[[nodiscard]] std::optional<size_t> columnOf(KanbanItemId id) const;

	protected:
		void onEvent(Event& event) override;
		void filterDescendantEvent(Event& event) override;
		void onTick(halnf deltaMs) override;
		[[nodiscard]] Extent measureContent(Axis, halnf) const override { return {}; }
		[[nodiscard]] const Widget* raisedChild() const override;
		void paintRaisedBackdrop(Painter& painter, const RectF& childRect) override;
		[[nodiscard]] bool acceptsKeyboardFocus() const override { return true; }

	private:
		class BoardLayout;
		class StripLayout;
		friend class BoardLayout;
		friend class StripLayout;

		struct Press {
			KanbanCard* card = nullptr;
			std::optional<size_t> column;
			Vec2F windowPoint = { 0, 0 };
			bool held = false;
		};

		struct Lift {
			Widget* widget = nullptr;
			Vec2F grabOffset = { 0, 0 };
			Vec2F size = { 0, 0 };
			Vec2F position = { 0, 0 };
			Vec2F pointer = { 0, 0 };
			Widget* focusBeforeLift = nullptr;
		};

		struct Drag {
			KanbanCard* card = nullptr;
			size_t fromColumn = 0;
			size_t fromIndex = 0;
			Lift lift;
			std::optional<KanbanDropTarget> target;
		};

		struct ColumnDrag {
			size_t column = 0;
			size_t destination = 0;
			Lift lift;
		};

		struct Landing {
			Widget* widget = nullptr;
			RectF from = makeRect(0, 0, 0, 0);
		};

		void rebuildColumns();
		void fillOptionsMenu(Menu& menu);
		[[nodiscard]] std::optional<KanbanItemId> morphingItem() const;
		[[nodiscard]] bool isFullViewShown() const { return morphingItem().has_value(); }
		void fullViewChanged();
		void synchronize(std::unordered_map<KanbanCard*, RectF> landings);
		void setColumnCards(
			const std::vector<std::vector<KanbanItemId>>& columnItems, std::unordered_map<KanbanCard*, RectF> landings
		);
		void removeVanishedCards(const std::vector<std::vector<KanbanItemId>>& columnItems);
		[[nodiscard]] std::optional<std::pair<size_t, size_t>> locate(KanbanItemId id) const;
		KanbanCard& cardFor(KanbanItemId id);
		void updateCard(KanbanCard& card);

		void scrollColumnsSideways(Event& event);
		void handlePointer(Event& event);
		void handleClick(Event& event);
		void handleKey(Event& event);
		[[nodiscard]] std::optional<KanbanItemId> neighbourOfSelection(InputID key) const;
		[[nodiscard]] std::optional<KanbanItemId> firstCard() const;
		[[nodiscard]] KanbanCard* cardAround(const Widget* target) const;
		[[nodiscard]] std::optional<size_t> columnHeaderAround(const Widget* target) const;
		Lift lift(Widget& widget, Vec2F windowPoint);
		void drop(const Lift& lifted);
		[[nodiscard]] const Lift* currentLift() const;
		void beginDrag();
		void beginColumnDrag();
		void updateDrag(Vec2F windowPoint);
		void finishDrag(bool commit);
		void finishColumnDrag(bool commit);
		[[nodiscard]] size_t columnDestinationAt(halnf liftedLeft) const;
		[[nodiscard]] std::optional<size_t> columnGapSlot() const;
		void autoScroll(halnf deltaMs);
		void refreshOptionsMenus();
		void showDropTarget(const std::optional<KanbanDropTarget>& target);
		[[nodiscard]] std::optional<size_t> columnAt(halnf windowX) const;
		[[nodiscard]] std::optional<KanbanDropTarget> dropTargetAt(Vec2F windowPoint) const;
		[[nodiscard]] bool accepts(size_t column, size_t index) const;
		[[nodiscard]] KanbanMove moveTo(size_t column, size_t index) const;
		[[nodiscard]] bool isNoOp(const KanbanMove& move) const;

		const KanbanModel& mModel;
		std::vector<KanbanColumn> mColumns;
		std::vector<std::unique_ptr<KanbanColumnView>> mColumnViews;
		ScrollView mColumnScroller{ Axis::Horizontal };
		Widget mColumnStrip;
		halnf mMinimumColumnWidth = 220;
		halnf mMaximumColumnWidth = 340;
		std::optional<halnf> mCardHeight;

		std::unordered_map<KanbanItemId, std::unique_ptr<KanbanCard>> mCards;
		CardFactory mCardFactory;
		CardUpdater mCardUpdater;
		std::optional<KanbanItemId> mSelection;
		std::function<void(KanbanItemId)> mOnSelected;
		std::function<void(KanbanItemId)> mOnActivated;
		std::function<void(const KanbanMove&)> mOnMoved;
		std::function<void(KanbanItemId)> mOnDeleteRequested;
		std::function<void(size_t, bool)> mOnColumnVisibilityChanged;
		std::function<void(size_t, size_t)> mOnColumnMoved;

		std::optional<Press> mPress;
		std::optional<Drag> mDrag;
		std::optional<ColumnDrag> mColumnDrag;
		std::optional<Landing> mColumnLanding;
		RectF mColumnGapRect = makeRect(0, 0, 0, 0);
		bool mSuppressNextClick = false;

		FullViewFrame mFrame;
		std::optional<KanbanItemId> mFull;
		std::optional<KanbanItemId> mFullSettling;
		RectF mFullOrigin = makeRect(0, 0, 0, 0);
		std::function<void(std::optional<KanbanItemId>)> mOnFullViewChanged;
	};
}

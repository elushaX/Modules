#pragma once

#include "Animation.hpp"
#include "Button.hpp"
#include "Containers.hpp"
#include "ResizeHandle.hpp"
#include "ScrollPhysics.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	class SplitView : public Widget {
	public:
		explicit SplitView(Axis axis = Axis::Horizontal);

		void setFirst(Widget* first);
		void setSecond(Widget* second);
		[[nodiscard]] Widget* first() const { return mFirst; }
		[[nodiscard]] Widget* second() const { return mSecond; }

		void setRatio(halnf ratio);
		[[nodiscard]] halnf ratio() const { return mRatio; }
		void setMinimumPaneLength(halnf length);

		[[nodiscard]] RectF handleRect() const;
		[[nodiscard]] ResizeHandle& handle() { return mHandle; }
		[[nodiscard]] bool isDragging() const { return mHandle.isDragging(); }

	protected:
		[[nodiscard]] bool hitTest(Vec2F) const override { return false; }

	private:
		class SplitLayout;

		void replaceChild(Widget*& slot, Widget* widget);
		[[nodiscard]] halnf ratioAt(halnf alongAxis) const;

		Axis mAxis;
		Widget* mFirst = nullptr;
		Widget* mSecond = nullptr;
		ResizeHandle mHandle;
		halnf mRatio = 0.5f;
		halnf mMinimumPaneLength = 40;
	};

	class TabView : public Widget {
	public:
		enum class Style { Tabs, Carousel };
		enum class TabAlignment { Start, Center };

		explicit TabView(Style style = Style::Tabs);

		void addTab(std::string title, Widget& page);
		void removeTab(Widget& page);

		void setCurrentIndex(size_t index);
		[[nodiscard]] long currentIndex() const { return mCurrent; }
		[[nodiscard]] size_t tabCount() const { return mTabs.size(); }
		[[nodiscard]] Button& tabButton(size_t index) { return *mTabs[index].button; }
		[[nodiscard]] ScrollView& tabScroller() { return mTabScroller; }
		void revealTab(size_t index);

		[[nodiscard]] Style style() const { return mStyle; }
		void setTabAlignment(TabAlignment alignment);
		[[nodiscard]] TabAlignment tabAlignment() const { return mTabAlignment; }
		void showNext();
		void showPrevious();
		[[nodiscard]] GlyphButton& previousButton() { return mPrevious; }
		[[nodiscard]] GlyphButton& nextButton() { return mNext; }
		[[nodiscard]] bool isSliding() const { return mStep.isActive(); }
		[[nodiscard]] bool isPulling() const { return mStep.isPulling(); }
		[[nodiscard]] halnf scrollPosition() const { return mStep.position(); }
		[[nodiscard]] RectF selectionIndicatorRect() const;

		void setOnChanged(std::function<void(size_t)> onChanged) { mOnChanged = std::move(onChanged); }

		static constexpr halnf kPagesPerTrackpadUnit = 0.15f;

	protected:
		void onEvent(Event& event) override;
		void onTick(halnf deltaMs) override;
		[[nodiscard]] bool isTransitioning() const override { return isSliding(); }
		void finishTransitions() override;

	private:
		class CarouselLayout;
		class CarouselTabBar;
		class CarouselTabButton;

		struct Tab {
			std::unique_ptr<Button> button;
			Widget* page;
			std::unique_ptr<Widget> slot;
		};

		struct ShownPage {
			size_t index;
			halnf offset;
		};

		struct Indicator {
			RectF rect;
			halnf opacity;
		};

		void buildTabsHeader();
		void buildCarouselHeader();
		[[nodiscard]] std::unique_ptr<Button> makeTabButton(std::string title, size_t index);
		[[nodiscard]] Widget& holder(const Tab& tab) const { return tab.slot ? *tab.slot : *tab.page; }
		[[nodiscard]] size_t cyclicIndex(long index) const;
		[[nodiscard]] bool slides() const { return mStyle == Style::Carousel && ui() != nullptr; }
		void showOnly(size_t index);
		void scrollBy(long pages);
		void pull(halnf pages);
		void settleOnCommitted();
		void positionChanged();
		void currentChanged(size_t index);
		[[nodiscard]] std::vector<ShownPage> shownPages() const;
		[[nodiscard]] std::vector<Indicator> indicators() const;
		[[nodiscard]] bool handleWheel(const Event& event);

		Style mStyle;
		TabAlignment mTabAlignment = TabAlignment::Start;
		std::unique_ptr<Panel> mTabBar;
		ScrollView mTabScroller{ Axis::Horizontal };
		Widget mHeader;
		GlyphButton mPrevious{ GlyphButton::Glyph::Previous };
		GlyphButton mNext{ GlyphButton::Glyph::Next };
		Panel mPageArea;
		std::vector<Tab> mTabs;
		long mCurrent = -1;
		MagneticStep mStep;
		std::function<void(size_t)> mOnChanged;
	};

	Vec2F saturationValueAt(Vec2F unitTrianglePoint);

	class SwatchGrid : public Control {
	public:
		static constexpr halnf kCell = 32;
		static constexpr halnf kGap = 6;

		SwatchGrid();

		void setColors(std::vector<RGBA> colors);
		[[nodiscard]] const std::vector<RGBA>& colors() const { return mColors; }
		void setCaptions(std::vector<std::string> captions);
		void setSelected(long index);
		[[nodiscard]] long selected() const { return mSelected; }
		[[nodiscard]] RectF cellRect(size_t index) const;
		[[nodiscard]] long indexAt(Vec2F localPoint) const;
		[[nodiscard]] size_t columns() const;

		void setOnPicked(std::function<void(size_t)> onPicked) { mOnPicked = std::move(onPicked); }
		void setOnHeld(std::function<void(size_t)> onHeld) { mOnHeld = std::move(onHeld); }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;

	private:
		[[nodiscard]] size_t columnsFor(halnf width) const;

		std::vector<RGBA> mColors;
		std::vector<std::string> mCaptions;
		long mSelected = -1;
		long mPressed = -1;
		std::function<void(size_t)> mOnPicked;
		std::function<void(size_t)> mOnHeld;
	};

	class ColorSquare : public Control {
	public:
		static constexpr halnf kHueStripHeight = 16;
		static constexpr halnf kGap = 8;

		ColorSquare();

		void setHsv(const HSV& hsv);
		[[nodiscard]] const HSV& hsv() const { return mHsv; }
		[[nodiscard]] RGBA color() const;
		[[nodiscard]] RectF squareRect() const;
		[[nodiscard]] RectF hueRect() const;
		[[nodiscard]] Vec2F windowPointFor(halnf saturation, halnf value) const;
		[[nodiscard]] Vec2F windowPointForHue(halnf hueRadians) const;

		void setOnChanged(std::function<void(const HSV&)> onChanged) { mOnChanged = std::move(onChanged); }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;

	private:
		enum class DragTarget { None, Square, Hue };

		void pickAt(Vec2F localPoint);

		HSV mHsv;
		DragTarget mDragTarget = DragTarget::None;
		std::function<void(const HSV&)> mOnChanged;
	};

	class ColorWheel : public Control {
	public:
		enum class DragTarget { None, Hue, SaturationValue };

		ColorWheel();

		void setColor(const RGBA& color);
		[[nodiscard]] RGBA color() const;
		[[nodiscard]] const HSV& hsv() const { return mHsv; }
		[[nodiscard]] bool isDragging() const { return mDragTarget != DragTarget::None; }

		void setOnChanged(std::function<void(const RGBA&)> onChanged) { mOnChanged = std::move(onChanged); }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;

	private:
		[[nodiscard]] RectF wheelRect() const;
		[[nodiscard]] DragTarget targetAt(Vec2F localPoint) const;
		void pickAt(Vec2F localPoint);

		HSV mHsv;
		DragTarget mDragTarget = DragTarget::None;
		std::function<void(const RGBA&)> mOnChanged;
	};
}

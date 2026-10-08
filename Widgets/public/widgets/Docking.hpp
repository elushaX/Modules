#pragma once

#include "Animation.hpp"
#include "Button.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "ResizeHandle.hpp"

#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	enum class DockSide { Left = 0, Right = 1, Bottom = 2 };

	class DockArea;

	class StripeButton : public Button {
	public:
		explicit StripeButton(std::string title, ImageId icon = {});

		[[nodiscard]] halnf stripeLength() const;

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		[[nodiscard]] RGBA labelColor() const;
		[[nodiscard]] halnf iconLength() const;
	};

	class OptionsButton : public PopupButton {
	public:
		OptionsButton();

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		[[nodiscard]] RGBA backgroundTarget() const override;
	};

	class DockPanel : public Panel {
	public:
		enum class ViewMode { Docked, Undocked, Float };

		explicit DockPanel(std::string title, DockSide side = DockSide::Left);
		~DockPanel() override;

		void setContent(Widget* content);
		[[nodiscard]] Widget* content() const { return mContent; }

		void setIcon(ImageId icon) { mIcon = icon; }
		[[nodiscard]] ImageId icon() const { return mIcon; }

		[[nodiscard]] const std::string& title() const { return mTitle.text(); }
		[[nodiscard]] DockSide side() const { return mSide; }
		[[nodiscard]] ViewMode viewMode() const { return mViewMode; }
		[[nodiscard]] bool isOpen() const { return mOpen; }
		void setOnOpenChanged(std::function<void(bool open)> callback) { mOnOpenChanged = std::move(callback); }
		[[nodiscard]] bool isDragging() const { return mDragging; }

		void setDockedLength(halnf length);
		[[nodiscard]] halnf dockedLength() const { return mDockedLength; }
		void setFloatingRect(const RectF& rect);
		[[nodiscard]] const RectF& floatingRect() const { return mFloatingRect; }

		[[nodiscard]] PopupButton& optionsButton() { return mOptionsButton; }
		[[nodiscard]] Menu& optionsMenu() { return mOptionsMenu; }
		[[nodiscard]] Button& hideButton() { return mHideButton; }
		[[nodiscard]] RectF headerRect() const { return mHeader.rect(); }
		[[nodiscard]] RectF resizeGripRect() const;
		[[nodiscard]] Widget& cornerGrip() { return mCornerGrip; }

	protected:
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;
		void paintOverlay(Painter& painter) override;
		void onActiveWithinChanged() override;

	private:
		class PanelLayout;

		class CornerGrip : public Widget {
		public:
			explicit CornerGrip(DockPanel& panel);

		protected:
			void onEvent(Event& event) override;
			void onHoverChanged() override;
			void onTick(halnf deltaMs) override;
			void paint(Painter& painter) override;

		private:
			void refreshReveal();

			DockPanel& mPanel;
			bool mDragging = false;
			Vec2F mGrabOffset = { 0, 0 };
			AnimatedFloat mReveal{ 0, Motion::quick() };
		};

		friend class DockArea;

		enum class Interaction { None, PendingDrag, Dragging };

		void buildOptionsMenu();
		void refreshChrome();
		void beginInteraction(Interaction interaction, const Event& event);
		void updateInteraction(const Event& event);
		void endInteraction(const Event& event);

		DockArea* mArea = nullptr;
		DockSide mSide;
		ViewMode mViewMode = ViewMode::Docked;
		bool mOpen = false;
		bool mDragging = false;
		ImageId mIcon;

		halnf mDockedLength = 260;
		RectF mFloatingRect = makeRect(120, 120, 280, 320);

		Interaction mInteraction = Interaction::None;
		Vec2F mGrabOffset = { 0, 0 };
		Vec2F mPressPosition = { 0, 0 };

		Widget mHeader;
		Label mTitle;
		OptionsButton mOptionsButton;
		GlyphButton mHideButton{ GlyphButton::Glyph::Hide };
		Menu mOptionsMenu;
		Menu mViewModeMenu;
		Menu mMoveToMenu;
		Widget mBody;
		Widget* mContent = nullptr;
		CornerGrip mCornerGrip{ *this };
		std::function<void(bool)> mOnOpenChanged;
	};

	class DockArea : public Widget {
	public:
		DockArea();
		~DockArea() override;

		void setCenter(Widget* center);
		[[nodiscard]] Widget* center() const { return mCenter; }

		void addPanel(DockPanel& panel);
		void removePanel(DockPanel& panel);
		[[nodiscard]] const std::vector<DockPanel*>& panels() const { return mPanels; }

		void openPanel(DockPanel& panel);
		void hidePanel(DockPanel& panel);
		void togglePanel(DockPanel& panel);
		void dockPanel(DockPanel& panel, DockSide side);
		void setViewMode(DockPanel& panel, DockPanel::ViewMode mode);

		[[nodiscard]] DockPanel* openPanelOn(DockSide side) const;
		[[nodiscard]] StripeButton* stripeButton(const DockPanel& panel) const;
		[[nodiscard]] RectF stripeRect(DockSide side) const;
		[[nodiscard]] ResizeHandle& edgeHandle(DockSide side) { return *mEdgeHandles[(int) side]; }
		[[nodiscard]] RectF centerRect() const;
		[[nodiscard]] bool isDraggingPanel() const { return mDragged != nullptr; }
		[[nodiscard]] bool isPreviewingDrop() const { return mDropSide >= 0; }
		[[nodiscard]] RectF dropPreview() const { return mDropPreview.value(); }
		[[nodiscard]] RectF dropTarget(DockSide side) const;

	protected:
		void paint(Painter& painter) override;
		void paintOverlay(Painter& painter) override;
		[[nodiscard]] bool hitTest(Vec2F localPoint) const override;

	private:
		class DockLayout;
		friend class DockPanel;

		struct Stripe {
			std::unique_ptr<StripeButton> button;
			DockPanel* panel;
		};

		struct Geometry {
			RectF stripes[2];
			RectF inner;
			RectF zones[3];
			RectF gaps[3];
			RectF center;
		};

		[[nodiscard]] Geometry computeGeometry(const RectF& area) const;
		[[nodiscard]] RectF panelRect(const DockPanel& panel, const Geometry& geometry, const RectF& area) const;
		[[nodiscard]] bool hasStripe(DockSide side) const;
		[[nodiscard]] halnf stripeThickness() const;
		[[nodiscard]] halnf clampDockedLength(halnf length, halnf extent) const;
		[[nodiscard]] long dropSideAt(Vec2F localPoint) const;
		[[nodiscard]] RectF zonePreview(const DockPanel& panel, DockSide side) const;
		[[nodiscard]] RectF openingRect(const DockPanel& panel) const;

		void beginDirectResize(DockPanel& panel);
		void endDirectResize(DockPanel& panel);
		void edgeDragged(DockSide side, halnf windowCoordinate);
		void updateDropPreview(const DockPanel& panel, Vec2F localPointer);

		void closeOthersOnSide(const DockPanel& panel);
		void rebuildStripes();
		void refreshStripeSelection();
		void refreshEdgeHandles();

		void panelDragStarted(DockPanel& panel);
		void panelDragged(DockPanel& panel, Vec2F windowPointer);
		void panelDragEnded(DockPanel& panel, Vec2F windowPointer);
		void floatResized(DockPanel& panel, Vec2F localBottomRight);
		void panelLostActivity(DockPanel& panel);

		Widget* mCenter = nullptr;
		std::vector<DockPanel*> mPanels;
		std::vector<Stripe> mStripes;
		std::unique_ptr<ResizeHandle> mEdgeHandles[3];

	protected:
		void onTick(halnf deltaMs) override;

	private:
		DockPanel* mDragged = nullptr;
		long mDropSide = -1;
		AnimatedRect mDropPreview{ makeRect(0, 0, 0, 0), Motion::fluid(), AnimatedRect::kGeometryTolerance };
		AnimatedFloat mDropPreviewOpacity{ 0, Motion::quick() };
	};
}

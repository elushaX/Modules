#pragma once

#include "Button.hpp"
#include "WindowControls.hpp"
#include "Containers.hpp"
#include "Popup.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	class WindowHost;

	class PopupButton : public Button {
	public:
		explicit PopupButton(std::string text = {});

		void setPopupContent(Widget* content);
		void setPopupOptions(const PopupOptions& options) { mOptions = options; }
		[[nodiscard]] PopupOptions& popupOptions() { return mOptions; }

		void openPopup();
		void openPopupFromHover();
		void closePopup();
		[[nodiscard]] bool isPopupOpen() const;

		void setOpenOnHover(bool openOnHover) { mOpenOnHover = openOnHover; }
		void setOnHovered(std::function<void()> onHovered) { mOnHovered = std::move(onHovered); }
		void setOnOpening(std::function<void()> onOpening) { mOnOpening = std::move(onOpening); }

	protected:
		void onEvent(Event& event) override;
		void onActivated() override;
		void onHoverChanged() override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		Widget* mPopupContent = nullptr;
		PopupOptions mOptions;
		bool mOpenOnHover = false;
		bool mOpenedByHover = false;
		bool mKeepOpenOnActivation = false;
		Vec2F mHoverOpenPoint = { 0, 0 };
		std::function<void()> mOnHovered;
		std::function<void()> mOnOpening;
	};

	class Menu;

	struct MenuEntry {
		enum class Kind { Action, Toggle, Separator, Submenu };

		Kind kind = Kind::Action;
		std::string text;
		std::string shortcut;
		std::function<void()> action;
		std::function<bool()> isChecked;
		std::function<bool()> isEnabled;
		std::function<std::string()> dynamicText;
		bool muted = false;
		ImageId icon;
		Menu* submenu = nullptr;

		static MenuEntry item(std::string text, std::function<void()> action, std::string shortcut = {});
		static MenuEntry toggle(std::string text, std::function<bool()> isChecked, std::function<void()> action);
		static MenuEntry separator();
		static MenuEntry nested(std::string text, Menu& menu);
	};

	class MenuItem : public Control {
	public:
		MenuItem(Menu& menu, MenuEntry entry);

		[[nodiscard]] const MenuEntry& entry() const { return mEntry; }
		[[nodiscard]] const std::string& text() const { return mText; }

		void refresh();

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		void onActivated() override;
		void onHoverChanged() override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		void openSubmenu();
		[[nodiscard]] Placement submenuSide(const RectF& menuRect) const;

		Menu& mMenu;
		MenuEntry mEntry;
		std::string mText;
	};

	class Menu : public Panel {
	public:
		Menu();
		~Menu() override;

		MenuItem& add(MenuEntry entry);
		MenuItem& addItem(std::string text, std::function<void()> action, std::string shortcut = {});
		MenuItem& addToggle(std::string text, std::function<bool()> isChecked, std::function<void()> action);
		void addSeparator();
		MenuItem& addSubmenu(std::string text, Menu& submenu);
		void clear();

		[[nodiscard]] const std::vector<std::unique_ptr<MenuItem>>& items() const { return mItems; }
		[[nodiscard]] std::vector<const MenuItem*> itemsWithSeparators() const;
		void notifyItemActivated() const {
			if (mOnItemActivated) mOnItemActivated();
		}

		void setOnItemActivated(std::function<void()> onItemActivated) { mOnItemActivated = std::move(onItemActivated); }
		void setOnOpen(std::function<void(Menu&)> onOpen) { mOnOpen = std::move(onOpen); }
		void refresh();

		void openAt(Widget& owner, const RectF& anchor, Placement placement);
		void closeMenu();
		[[nodiscard]] bool isOpen() const;

	private:
		friend class MenuItem;

		void closeChain();

		ScrollView mScroll;
		Widget mColumn;
		std::vector<std::unique_ptr<MenuItem>> mItems;
		std::vector<std::unique_ptr<Separator>> mSeparators;
		std::function<void()> mOnItemActivated;
		std::function<void(Menu&)> mOnOpen;
	};

	class MenuIconButton : public PopupButton {
	public:
		MenuIconButton();

	protected:
		[[nodiscard]] bool hasIcon() const override { return true; }
		void paintIcon(Painter& painter, const RectF& box) override;
	};

	class MenuBar : public Panel {
	public:
		MenuBar();

		PopupButton& addMenu(std::string title, Menu& menu);
		PopupButton& addTrailingMenu(std::string title, Menu& menu);
		void removeMenu(Menu& menu);
		void setCenterWidget(Widget* widget);
		void setTrailingWidget(Widget* widget);
		[[nodiscard]] Widget* trailingWidget() const { return mTrailing; }
		[[nodiscard]] Widget* centerWidget() const { return mCenter; }
		[[nodiscard]] const std::vector<std::unique_ptr<PopupButton>>& titles() const { return mTitles; }
		[[nodiscard]] WindowControls& windowControls() { return mWindowControls; }
		[[nodiscard]] PopupButton& overflowButton() { return mOverflow; }
		[[nodiscard]] Menu& overflowMenu() { return mOverflowMenu; }
		[[nodiscard]] size_t shownTitleCount() const { return mShownTitles; }

	protected:
		void onEvent(Event& event) override;

	private:
		class MenuBarLayout;
		friend class MenuBarLayout;

		PopupButton& insertMenu(size_t index, std::string title, Menu& menu);
		void showTitles(size_t count);
		void onTitleHovered(PopupButton& title);
		[[nodiscard]] bool isDragSurface(const Widget* target) const;
		void handleDragSurfacePress(Event& event, WindowHost& host);
		void handlePendingMove(Event& event, WindowHost& host);
		void cancelPendingMove();

		std::vector<std::unique_ptr<PopupButton>> mTitles;
		bool mMovePending = false;
		Vec2F mMovePressPoint = { 0, 0 };
		Spacer mDragSpace;
		Widget* mCenter = nullptr;
		Widget* mTrailing = nullptr;
		size_t mTrailingMenus = 0;
		WindowControls mWindowControls;
		PopupButton mOverflow{ "\u00bb" };
		Menu mOverflowMenu;
		std::vector<Menu*> mMenus;
		size_t mShownTitles = 0;
	};

	class Dropdown : public PopupButton {
	public:
		explicit Dropdown(std::vector<std::string> options = {});

		void setOptions(std::vector<std::string> options);
		[[nodiscard]] const std::vector<std::string>& options() const { return mOptions; }

		void setSelectedIndex(long index);
		[[nodiscard]] long selectedIndex() const { return mSelected; }
		[[nodiscard]] std::string selectedText() const;

		void setOnSelected(std::function<void(long)> onSelected) { mOnSelected = std::move(onSelected); }

		[[nodiscard]] Menu& menu() { return mMenu; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		void onActivated() override;

	private:
		void rebuildMenu();

		std::vector<std::string> mOptions;
		long mSelected = -1;
		Menu mMenu;
		std::function<void(long)> mOnSelected;
	};
}

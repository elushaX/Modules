#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "PageStack.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	class SettingsView;

	class SettingsRow : public Control {
	public:
		SettingsRow(SettingsView& view, Menu& menu, const MenuItem& item);

		[[nodiscard]] const MenuEntry& entry() const { return mEntry; }
		[[nodiscard]] const std::string& text() const { return mText; }
		void refresh();

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		void onActivated() override;

	private:
		SettingsView& mView;
		Menu& mMenu;
		const MenuEntry& mEntry;
		std::string mText;
	};

	class SettingsPage : public StackPage {
	public:
		SettingsPage(SettingsView& view, std::string title);

		[[nodiscard]] Menu* menu() const { return mMenu; }
		[[nodiscard]] const std::vector<std::unique_ptr<SettingsRow>>& rows() const { return mRows; }
		[[nodiscard]] SettingsRow* row(const std::string& text) const;

		void showMenu(Menu& menu);
		void showContent(Widget& content);
		void refresh();
		void refreshRows();

	private:
		void clear();

		SettingsView& mView;
		Menu* mMenu = nullptr;
		std::vector<std::unique_ptr<SettingsRow>> mRows;
		std::vector<std::unique_ptr<Widget>> mSections;
	};

	class SettingsView : public PageStack {
	public:
		SettingsView();
		~SettingsView() override;

		void setRoot(std::string title, Menu& menu);
		void setRootContent(std::string title, Widget& content);
		void setOnPageShown(std::function<void(SettingsPage&)> onPageShown);
		void open(Ui& ui);
		void close();
		[[nodiscard]] bool isOpen() const;

		void openMenu(std::string title, Menu& menu);
		void openContent(std::string title, Widget& content);

		[[nodiscard]] SettingsPage& currentPage() const { return static_cast<SettingsPage&>(PageStack::currentPage()); }

		void setOnClosed(std::function<void()> onClosed) { mOnClosed = std::move(onClosed); }
		void setOnContentRequested(std::function<bool(const MenuEntry&)> onContentRequested) {
			mOnContentRequested = std::move(onContentRequested);
		}

	protected:
		[[nodiscard]] std::unique_ptr<StackPage> makePage(std::string title) override;
		[[nodiscard]] bool animatesPages() const override { return isOpen(); }
		void pageRevealed(StackPage& page) override { static_cast<SettingsPage&>(page).refresh(); }

	private:
		friend class SettingsRow;

		void activated(Menu& menu, const MenuEntry& entry);

		std::string mRootTitle;
		Menu* mRootMenu = nullptr;
		Widget* mRootContent = nullptr;
		std::function<void()> mOnClosed;
		std::function<bool(const MenuEntry&)> mOnContentRequested;
	};
}

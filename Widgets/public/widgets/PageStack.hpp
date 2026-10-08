#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Label.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	class PageStack;

	class StackPage : public Widget {
	public:
		StackPage(PageStack& stack, std::string title);

		[[nodiscard]] const std::string& title() const { return mTitle.text(); }
		void setTitle(std::string title) { mTitle.setText(std::move(title)); }
		[[nodiscard]] Widget& header() { return mHeader; }
		[[nodiscard]] Widget& body() { return mBody; }
		[[nodiscard]] Button& backButton() { return mBack; }
		[[nodiscard]] const Label& titleLabel() const { return mTitle; }

		void setMaximumWidth(halnf width);
		void setContent(Widget& content);
		void setScrollsContent(bool scrolls);
		[[nodiscard]] bool scrollsContent() const { return mScrollsContent; }
		void setHeaderContent(Widget* content);
		void clearContent() { mBody.removeAllChildren(); }

	private:
		[[nodiscard]] AxisSizing horizontalSizing() const;

		PageStack& mStack;
		halnf mMaximumWidth = 0;
		bool mScrollsContent = true;
		Widget mHeader;
		Button mBack{ "Back" };
		Label mTitle;
		ScrollView mScroll;
		Widget mBody;
	};

	class PageStack : public Panel {
	public:
		explicit PageStack(Surface surface = Surface::None);
		~PageStack() override;

		StackPage& push(std::string title);
		StackPage& push(std::string title, Widget& content);
		[[nodiscard]] std::vector<std::string> titles() const;
		void back();
		void backTo(size_t depth);
		void clear();

		[[nodiscard]] size_t depth() const { return mPages.size(); }
		[[nodiscard]] StackPage& currentPage() const { return *mPages.back(); }
		[[nodiscard]] StackPage& page(size_t index) const { return *mPages[index]; }

		void setPageMaximumWidth(halnf width) { mPageMaximumWidth = width; }
		void setPagePadding(Insets padding);
		void setRootBackText(std::string text) { mRootBackText = std::move(text); }
		void setHeaderContent(Widget* content);
		void setPageHeadersShown(bool shown);
		[[nodiscard]] bool pageHeadersShown() const { return mPageHeadersShown; }
		void setOnPageShown(std::function<void(StackPage&)> callback) { mOnPageShown = std::move(callback); }
		void setOnBackFromRoot(std::function<void()> callback) { mOnBackFromRoot = std::move(callback); }

	protected:
		void onEvent(Event& event) override;
		[[nodiscard]] virtual std::unique_ptr<StackPage> makePage(std::string title);
		[[nodiscard]] virtual bool animatesPages() const { return ui() != nullptr; }
		virtual void pageRevealed(StackPage&) {}

	private:
		void showTop(halnf fromDirection);
		void popTo(size_t depth);

		std::vector<std::unique_ptr<StackPage>> mPages;
		std::vector<std::unique_ptr<StackPage>> mRetiredPages;
		std::function<void(StackPage&)> mOnPageShown;
		std::function<void()> mOnBackFromRoot;
		std::string mRootBackText = "Back";
		Widget* mHeaderContent = nullptr;
		halnf mPageMaximumWidth = 0;
		Insets mPagePadding;
		bool mPageHeadersShown = true;
	};
}

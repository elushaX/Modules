#include "PageStack.hpp"

#include "BoxLayout.hpp"
#include "StackLayout.hpp"
#include "Ui.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kHeaderGap = 12;
	constexpr halnf kForward = 1.0f;
	constexpr halnf kBackward = -0.3f;
}

StackPage::StackPage(PageStack& stack, std::string title) :
	mStack(stack),
	mTitle(std::move(title)) {
	setDebugName("stack page: " + mTitle.text());
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	emplaceLayout<ColumnLayout>(kHeaderGap).setCrossAlign(CrossAlign::Center);

	mHeader.setDebugName("stack page header");
	mHeader.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mHeader.emplaceLayout<RowLayout>(kHeaderGap).setCrossAlign(CrossAlign::Center);
	mBack.setStyle(Button::Style::Flat);
	mBack.setOnActivate([this]() { mStack.back(); });
	mTitle.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mTitle.setAlignment(TextAlign::End);
	mHeader.addChild(mBack);
	mHeader.addChild(mTitle);

	mBody.setDebugName("stack page body");
	mBody.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mBody.emplaceLayout<ColumnLayout>(16).setPadding(Insets::symmetric(0, 4));
	mScroll.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mScroll.setContent(&mBody);

	addChild(mHeader);
	addChild(mScroll);
}

AxisSizing StackPage::horizontalSizing() const {
	return mMaximumWidth > 0 ? AxisSizing::fill().atMost(mMaximumWidth) : AxisSizing::fill();
}

void StackPage::setMaximumWidth(halnf width) {
	mMaximumWidth = width;
	mHeader.setSizing(horizontalSizing(), AxisSizing::fit());
	mScroll.setSizing(horizontalSizing(), AxisSizing::fill());
	if (!mScrollsContent) mBody.setSizing(horizontalSizing(), AxisSizing::fill());
}

void StackPage::setScrollsContent(bool scrolls) {
	if (scrolls == mScrollsContent) return;
	mScrollsContent = scrolls;

	if (scrolls) {
		removeChild(mBody);
		mBody.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mScroll.setContent(&mBody);
		addChild(mScroll);
		return;
	}

	mScroll.setContent(nullptr);
	removeChild(mScroll);
	mBody.setSizing(horizontalSizing(), AxisSizing::fill());
	addChild(mBody);
}

void StackPage::setHeaderContent(Widget* content) {
	mHeader.removeAllChildren();
	mHeader.addChild(mBack);
	mHeader.addChild(content ? *content : static_cast<Widget&>(mTitle));
}

void StackPage::setContent(Widget& content) {
	mBody.removeAllChildren();
	mBody.addChild(content);
}

PageStack::PageStack(Surface surface) :
	Panel(surface) {
	setDebugName("page stack");
	setCornerRadius(0);
	emplaceLayout<StackLayout>();
}

PageStack::~PageStack() { removeAllChildren(); }

void PageStack::setPagePadding(Insets padding) {
	mPagePadding = padding;
	emplaceLayout<StackLayout>().setPadding(padding);
}

void PageStack::setPageHeadersShown(bool shown) {
	mPageHeadersShown = shown;
	for (auto& page : mPages)
		page->header().setVisible(shown);
}

void PageStack::setHeaderContent(Widget* content) {
	mHeaderContent = content;
	if (!mPages.empty()) mPages.back()->setHeaderContent(content);
}

std::unique_ptr<StackPage> PageStack::makePage(std::string title) {
	return std::make_unique<StackPage>(*this, std::move(title));
}

StackPage& PageStack::push(std::string title) {
	mRetiredPages.clear();
	mPages.push_back(makePage(std::move(title)));
	StackPage& page = *mPages.back();
	page.setMaximumWidth(mPageMaximumWidth);
	page.backButton().setText(mPages.size() == 1 ? mRootBackText : "Back");
	showTop(mPages.size() == 1 ? 0.0f : kForward);
	return page;
}

StackPage& PageStack::push(std::string title, Widget& content) {
	StackPage& page = push(std::move(title));
	page.setContent(content);
	return page;
}

std::vector<std::string> PageStack::titles() const {
	std::vector<std::string> titles;
	for (const auto& page : mPages)
		titles.push_back(page->title());
	return titles;
}

void PageStack::back() {
	if (mPages.size() <= 1) {
		if (mOnBackFromRoot) mOnBackFromRoot();
		return;
	}
	popTo(mPages.size() - 1);
}

void PageStack::backTo(size_t targetDepth) {
	if (targetDepth == 0 || targetDepth >= mPages.size()) return;
	popTo(targetDepth);
}

void PageStack::popTo(size_t targetDepth) {
	removeAllChildren();
	while (mPages.size() > targetDepth) {
		mRetiredPages.push_back(std::move(mPages.back()));
		mPages.pop_back();
	}
	pageRevealed(*mPages.back());
	if (mOnPageShown) mOnPageShown(*mPages.back());
	showTop(kBackward);
}

void PageStack::clear() {
	removeAllChildren();
	for (auto& page : mPages)
		mRetiredPages.push_back(std::move(page));
	mPages.clear();
}

void PageStack::showTop(halnf fromDirection) {
	removeAllChildren();
	if (mPages.empty()) return;

	StackPage& page = *mPages.back();
	page.setHeaderContent(mHeaderContent);
	page.header().setVisible(mPageHeadersShown);
	addChild(page);
	requestLayout();
	if (fromDirection == 0 || !animatesPages()) return;

	const RectF area = mPagePadding.shrink(localRect());
	page.appearFrom(makeRect(area.pos.x + area.size.x * fromDirection, area.pos.y, area.size.x, area.size.y));
}

void PageStack::onEvent(Event& event) {
	if (event.type() != Event::Type::KeyDown || event.keyCode() != InputID::ESCAPE) return;
	back();
	event.accept();
}

#include "BoxLayout.hpp"
#include "Breadcrumb.hpp"
#include "PageStack.hpp"
#include "SearchField.hpp"
#include "Ui.hpp"
#include "UiDriver.hpp"

#include "UnitTest++/UnitTest++.h"

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::testing;

namespace {
	struct StackScene {
		Widget root;
		PageStack stack;
		Label first{ "first page" };
		Label second{ "second page" };
		Label third{ "third page" };
		std::vector<std::string> shown;
		int backFromRoot = 0;

		StackScene() {
			root.emplaceLayout<ColumnLayout>();
			stack.setSizing(AxisSizing::fill(), AxisSizing::fill());
			root.addChild(stack);
			stack.setOnPageShown([this](StackPage& page) { shown.push_back(page.title()); });
			stack.setOnBackFromRoot([this]() { backFromRoot++; });
		}
	};

	struct SearchScene {
		Widget root;
		SearchField search;
		std::vector<std::string> items = { "Alpha", "Beta", "Gamma", "Alphabet" };
		std::vector<size_t> chosen;
		std::string disabled;

		SearchScene() {
			root.emplaceLayout<ColumnLayout>();
			root.addChild(search);
			search.setQuery([this](const std::string& query) {
				std::vector<SearchResult> results;
				if (query.empty()) return results;
				for (const std::string& item : items) {
					if (item.find(query) == std::string::npos) continue;
					results.push_back({ item, "detail of " + item, item != disabled });
				}
				return results;
			});
			search.setOnChosen([this](size_t index) { chosen.push_back(index); });
		}

		void type(UiDriver& driver, const std::string& text) {
			driver.click(search.input());
			driver.type(text);
			driver.settle();
		}
	};
}

SUITE(PageStackNavigation) {
	TEST(PushShowsNewestPageAndBackReturns) {
		StackScene scene;
		UiDriver driver(scene.root, { 800, 600 });

		scene.stack.push("Root", scene.first);
		scene.stack.push("Child", scene.second);
		driver.settle();

		CHECK_EQUAL(2u, scene.stack.depth());
		CHECK_EQUAL("Child", scene.stack.currentPage().title());
		CHECK(scene.second.ui() != nullptr);
		CHECK(scene.first.ui() == nullptr);

		driver.click(scene.stack.currentPage().backButton());
		driver.settle();
		CHECK_EQUAL(1u, scene.stack.depth());
		CHECK(scene.first.ui() != nullptr);
		REQUIRE CHECK_EQUAL(1u, scene.shown.size());
		CHECK_EQUAL("Root", scene.shown.front());
	}

	TEST(BackOnRootReportsInsteadOfPopping) {
		StackScene scene;
		UiDriver driver(scene.root, { 800, 600 });
		scene.stack.setRootBackText("Close");
		scene.stack.push("Root", scene.first);
		driver.settle();

		CHECK_EQUAL("Close", scene.stack.currentPage().backButton().text());
		driver.click(scene.stack.currentPage().backButton());
		CHECK_EQUAL(1, scene.backFromRoot);
		CHECK_EQUAL(1u, scene.stack.depth());
	}

	TEST(BackToJumpsSeveralLevelsAndIgnoresInvalidDepths) {
		StackScene scene;
		UiDriver driver(scene.root, { 800, 600 });
		scene.stack.push("Root", scene.first);
		scene.stack.push("A", scene.second);
		scene.stack.push("B", scene.third);

		scene.stack.backTo(0);
		scene.stack.backTo(3);
		CHECK_EQUAL(3u, scene.stack.depth());

		scene.stack.backTo(1);
		driver.settle();
		CHECK_EQUAL(1u, scene.stack.depth());
		CHECK(scene.stack.titles() == std::vector<std::string>{ "Root" });
	}

	TEST(EscapeFromAFocusedPageGoesBack) {
		StackScene scene;
		UiDriver driver(scene.root, { 800, 600 });
		scene.stack.push("Root", scene.first);
		scene.stack.push("Child", scene.second);
		driver.settle();

		Button inside{ "inside" };
		scene.stack.currentPage().setContent(inside);
		driver.settle();
		driver.click(inside);
		driver.key(InputID::ESCAPE);
		CHECK_EQUAL(1u, scene.stack.depth());
	}
}

SUITE(PageStackHeader) {
	TEST(HeaderContentReplacesTitleOnEveryShownPage) {
		StackScene scene;
		Label path{ "custom path" };
		scene.stack.setHeaderContent(&path);
		UiDriver driver(scene.root, { 800, 600 });
		scene.stack.push("Root", scene.first);
		scene.stack.push("Child", scene.second);
		driver.settle();

		CHECK(path.parent() == &scene.stack.currentPage().header());
		CHECK(scene.stack.currentPage().titleLabel().parent() == nullptr);

		scene.stack.back();
		driver.settle();
		CHECK(path.parent() == &scene.stack.currentPage().header());

		scene.stack.setHeaderContent(nullptr);
		CHECK(path.parent() == nullptr);
		CHECK(scene.stack.currentPage().titleLabel().parent() == &scene.stack.currentPage().header());
	}
}

SUITE(BreadcrumbParts) {
	TEST(LastPartIsCurrentAndOthersReportTheirIndex) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Breadcrumb crumbs;
		root.addChild(crumbs);
		std::vector<size_t> chosen;
		crumbs.setOnPartChosen([&](size_t index) { chosen.push_back(index); });
		crumbs.setParts({ "Board", "Widgets", "Text edit" });
		UiDriver driver(root, { 800, 200 });

		REQUIRE CHECK_EQUAL(3u, crumbs.partCount());
		CHECK(!crumbs.part(2).isEnabled());
		driver.click(crumbs.part(1));
		driver.click(crumbs.part(2));
		CHECK(chosen == std::vector<size_t>{ 1 });

		crumbs.setParts({ "Board" });
		CHECK_EQUAL(1u, crumbs.partCount());
	}

	TEST(OverflowingPathStaysClickable) {
		Widget root;
		root.emplaceLayout<ColumnLayout>();
		Breadcrumb crumbs;
		root.addChild(crumbs);
		std::vector<size_t> chosen;
		crumbs.setOnPartChosen([&](size_t index) { chosen.push_back(index); });
		crumbs.setParts({ "Board", "A long task title", "Another long title", "Current task" });
		UiDriver driver(root, { 220, 200 });

		const RectF part = crumbs.part(0).windowRect();
		CHECK(driver.ui().widgetAt(part.pos + part.size / 2.0f) == &crumbs.part(0));
		driver.click(crumbs.part(0));
		CHECK(chosen == std::vector<size_t>{ 0 });
	}
}

SUITE(SearchFieldResults) {
	TEST(TypingShowsMatchesAndClearingHidesThem) {
		SearchScene scene;
		UiDriver driver(scene.root, { 800, 600 });

		scene.type(driver, "Alph");
		REQUIRE CHECK(scene.search.isShowingResults());
		REQUIRE CHECK_EQUAL(2u, scene.search.rows().size());
		CHECK_EQUAL("Alpha", scene.search.rows()[0]->result().title);
		CHECK(scene.search.input().isEditing());

		scene.search.setText("zzz");
		CHECK(!scene.search.isShowingResults());
	}

	TEST(KeyboardSelectsAndEnterChooses) {
		SearchScene scene;
		UiDriver driver(scene.root, { 800, 600 });
		scene.type(driver, "a");

		CHECK_EQUAL(0, scene.search.highlighted());
		driver.key(InputID::DOWN);
		driver.key(InputID::DOWN);
		driver.key(InputID::UP);
		CHECK_EQUAL(1, scene.search.highlighted());
		driver.key(InputID::ENTER);

		CHECK(scene.chosen == std::vector<size_t>{ 1 });
		CHECK(!scene.search.isShowingResults());
	}

	TEST(TypingContinuesWhileResultsAreOpen) {
		SearchScene scene;
		UiDriver driver(scene.root, { 800, 600 });
		scene.type(driver, "Alph");
		driver.type("ab");
		driver.settle();

		CHECK_EQUAL("Alphab", scene.search.input().text());
		REQUIRE CHECK_EQUAL(1u, scene.search.rows().size());
		CHECK_EQUAL("Alphabet", scene.search.rows()[0]->result().title);
	}

	TEST(DisabledResultsAreSkippedAndCannotBeChosen) {
		SearchScene scene;
		scene.disabled = "Alpha";
		UiDriver driver(scene.root, { 800, 600 });
		scene.type(driver, "Alph");

		CHECK_EQUAL(1, scene.search.highlighted());
		driver.key(InputID::UP);
		CHECK_EQUAL(1, scene.search.highlighted());
		driver.click(*scene.search.rows()[0]);
		CHECK(scene.chosen.empty());

		driver.click(*scene.search.rows()[1]);
		CHECK(scene.chosen == std::vector<size_t>{ 1 });
	}

	TEST(EscapeClosesResultsAndKeepsQuery) {
		SearchScene scene;
		UiDriver driver(scene.root, { 800, 600 });
		scene.type(driver, "Beta");

		driver.key(InputID::ESCAPE);
		CHECK(!scene.search.isShowingResults());
		CHECK_EQUAL("Beta", scene.search.input().text());
		CHECK(scene.chosen.empty());
	}

	TEST(FocusingShowsResultsForEmptyQuery) {
		SearchScene scene;
		scene.search.setQuery([](const std::string& query) {
			return std::vector<SearchResult>{ { "Create: " + (query.empty() ? std::string("new") : query), "", true } };
		});
		UiDriver driver(scene.root, { 800, 600 });

		scene.search.focus();
		driver.settle();
		REQUIRE CHECK(scene.search.isShowingResults());
		CHECK_EQUAL("Create: new", scene.search.rows()[0]->result().title);
		CHECK(scene.search.input().isEditing());
	}

	TEST(ResultsCanBeWiderThanFieldAndStayCentred) {
		Widget root;
		root.emplaceLayout<ColumnLayout>().setCrossAlign(CrossAlign::Center);
		SearchField search;
		search.setSizing(AxisSizing::fixed(200), AxisSizing::fit());
		search.setMinimumResultsWidth(400);
		search.setQuery([](const std::string&) { return std::vector<SearchResult>{ { "Result", "", true } }; });
		root.addChild(search);
		UiDriver driver(root, { 800, 600 });

		search.focus();
		driver.settle();
		REQUIRE CHECK(search.isShowingResults());
		const RectF field = search.windowRect();
		const RectF results = search.rows()[0]->parent()->windowRect();
		CHECK_CLOSE(400.0f, results.size.x, 0.5f);
		CHECK_CLOSE(field.center().x, results.center().x, 0.5f);
	}

	TEST(MaximumResultsLimitsRows) {
		SearchScene scene;
		scene.search.setMaximumResults(2);
		UiDriver driver(scene.root, { 800, 600 });
		scene.type(driver, "a");
		CHECK_EQUAL(2u, scene.search.rows().size());
	}
}

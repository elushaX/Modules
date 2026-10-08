#include "BoxLayout.hpp"
#include "Event.hpp"
#include "FakeWindowHost.hpp"
#include "MarkdownEditor.hpp"
#include "MarkdownView.hpp"
#include "UiDriver.hpp"

#include <filesystem>
#include <unistd.h>
#include "UnitTest++/UnitTest++.h"

#include <cmath>
#include <map>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::markdown;
using namespace tp::ui::testing;

namespace {
	struct ViewScene {
		Widget root;
		MarkdownView view;

		explicit ViewScene(const std::string& source) {
			root.emplaceLayout<ColumnLayout>();
			view.setMarkdown(source);
			root.addChild(view);
		}
	};

	class FakeImageLoader : public ImageLoader {
	public:
		std::map<std::string, Vec2F> sizes;

		LoadedImage load(const std::string& path) override {
			const auto found = sizes.find(path);
			if (found == sizes.end()) return {};
			return { ImageId{ 100 + (ualni) path.size() }, found->second };
		}
	};

	const PaintCommand* textStartingWith(RecordingPainter& painter, const std::string& prefix) {
		for (const PaintCommand& command : painter.commands()) {
			if (command.kind == PaintCommand::Kind::Text && command.text.rfind(prefix, 0) == 0) return &command;
		}
		return nullptr;
	}

	Modifiers control() {
		Modifiers modifiers;
		modifiers.control = true;
		return modifiers;
	}

	std::string longDocument() {
		std::string source;
		for (int index = 0; index < 60; index++)
			source += "## Section " + std::to_string(index) + "\n\ntext\n\n";
		return source;
	}

	int sectionOf(const std::string& source, size_t offset) {
		const size_t heading = source.rfind("## Section ", offset);
		if (heading == std::string::npos) return -1;
		return std::stoi(source.substr(heading + 11));
	}

	struct EditorScene {
		Widget root;
		MarkdownEditor editor;
		std::vector<std::string> changes;

		explicit EditorScene(const std::string& source) {
			root.emplaceLayout<ColumnLayout>();
			root.setSizing(AxisSizing::fill(), AxisSizing::fill());
			editor.setMarkdown(source);
			editor.setOnChanged([this](const std::string& text) { changes.push_back(text); });
			root.addChild(editor);
		}
	};
}

SUITE(MarkdownViewWidget) {
	TEST(HeadingsAreBoldAndScaledByLevel) {
		ViewScene scene("# Big\n\n### Small\n\nbody");
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		const PaintCommand* big = painter.findText("Big");
		const PaintCommand* small = painter.findText("Small");
		const PaintCommand* body = painter.findText("body");
		CHECK(big && small && body);
		CHECK(big->face == FontFace::Bold);
		CHECK(big->fontSize > small->fontSize);
		CHECK(small->fontSize > body->fontSize - 0.01f);
		CHECK(big->rect.pos.y < small->rect.pos.y && small->rect.pos.y < body->rect.pos.y);
	}

	TEST(InlineStylesMapToFontFaces) {
		ViewScene scene("a **b** *c* ***d*** `e`");
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		CHECK(painter.findText("b")->face == FontFace::Bold);
		CHECK(painter.findText("c")->face == FontFace::Italic);
		CHECK(painter.findText("d")->face == FontFace::BoldItalic);
		CHECK(painter.findText("e")->face == FontFace::Monospace);
	}

	TEST(HardBreaksStartANewLineInThePreview) {
		ViewScene scene("first line.\\\nsecond line");
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		const PaintCommand* first = textStartingWith(painter, "first");
		const PaintCommand* second = textStartingWith(painter, "second");
		CHECK(first && second);
		CHECK(second->rect.pos.y > first->rect.pos.y);
	}

	TEST(HardBreakAfterALinkStartsANewLine) {
		ViewScene scene("see https://example.com.\\\nNext line");
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		const PaintCommand* next = textStartingWith(painter, "Next");
		const PaintCommand* see = textStartingWith(painter, "see");
		CHECK(next && see);
		if (next && see) CHECK(next->rect.pos.y > see->rect.pos.y);
	}

	TEST(ListsShowMarkersNumbersAndNesting) {
		ViewScene scene("4. four\n5. five\n   - inner\n");
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		CHECK(painter.hasText("4."));
		CHECK(painter.hasText("5."));
		size_t markers = 0;
		for (const PaintCommand& command : painter.commands())
			markers += command.kind == PaintCommand::Kind::Circle || command.kind == PaintCommand::Kind::StrokeCircle;
		CHECK_EQUAL(1u, markers);
		CHECK(painter.findText("inner")->rect.pos.x > painter.findText("five")->rect.pos.x);
	}

	TEST(TaskItemsUseCheckboxesThatRewriteTheSource) {
		ViewScene scene("- [ ] write\n- [x] read\n");
		std::string rewritten;
		scene.view.setOnTaskToggled([&](const std::string& source) { rewritten = source; });
		UiDriver driver(scene.root);

		std::vector<Checkbox*> boxes = scene.view.taskBoxes();
		CHECK_EQUAL(2u, boxes.size());
		CHECK(!boxes[0]->isChecked());
		CHECK(boxes[1]->isChecked());

		driver.click(*boxes[0]);
		driver.settle();

		CHECK_EQUAL("- [x] write\n- [x] read\n", rewritten);
		CHECK_EQUAL(rewritten, scene.view.markdown());
		CHECK(scene.view.taskBoxes()[0]->isChecked());
	}

	TEST(CodeBlocksKeepLinesAndUseMonospace) {
		ViewScene scene("```\nline one\n  line two\n```\n");
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		const PaintCommand* one = painter.findText("line one");
		const PaintCommand* two = painter.findText("  line two");
		CHECK(one && two);
		if (!one || !two) return;
		CHECK(one->face == FontFace::Monospace);
		CHECK(two->rect.pos.y > one->rect.pos.y);
	}

	TEST(TablesPlaceCellsInAlignedColumns) {
		ViewScene scene("| Key | Value |\n|-----|------:|\n| a | 1 |\n| longer key | 22 |\n");
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		const PaintCommand* header = painter.findText("Key");
		const PaintCommand* a = painter.findText("a");
		const PaintCommand* longer = painter.findText("longer key");
		const PaintCommand* value = painter.findText("Value");
		CHECK(header && a && longer && value);
		CHECK(header->face == FontFace::Bold);
		CHECK_CLOSE(header->rect.pos.x, a->rect.pos.x, 0.5f);
		CHECK_CLOSE(a->rect.pos.x, longer->rect.pos.x, 0.5f);
		CHECK(value->rect.pos.x > longer->rect.pos.x + MonospaceTextMetrics().textWidth("longer key", 13));
		CHECK(painter.findText("1")->rect.pos.x > painter.findText("22")->rect.pos.x);
	}

	TEST(LinksReportTheirTargetWhenClicked) {
		ViewScene scene("go to [site](https://example.com) now");
		std::string activated;
		scene.view.setOnLinkActivated([&](const std::string& target) { activated = target; });
		UiDriver driver(scene.root);

		const PaintCommand* site = driver.paint().findText("site");
		CHECK(site);
		driver.click(site->rect.center());

		CHECK_EQUAL("https://example.com", activated);
	}

	TEST(QuotesRulesAndAdmonitionsRender) {
		ViewScene scene("> wise words\n\n---\n\n> [!WARNING]\n> careful\n");
		UiDriver driver(scene.root);

		RecordingPainter& painter = driver.paint();
		CHECK(painter.hasText("wise words"));
		CHECK(painter.hasText("Warning"));
		CHECK(painter.hasText("careful"));
		bool rule = false;
		for (const PaintCommand& command : painter.commands())
			rule |= command.kind == PaintCommand::Kind::Line;
		CHECK(rule);
		CHECK_EQUAL(3u, scene.view.topBlocks().size());
	}

	TEST(SourceOffsetsMapToBlockPositionsBothWays) {
		const std::string source = "# One\n\npara\n\n## Two\n\nend\n";
		ViewScene scene(source);
		UiDriver driver(scene.root);

		const size_t two = source.find("Two");
		const std::optional<halnf> top = scene.view.offsetOfSource(two);
		CHECK(top.has_value());
		CHECK_CLOSE(scene.view.topBlocks()[2]->rect().pos.y, *top, 0.5f);

		const std::optional<size_t> back = scene.view.sourceAt(*top + 1);
		CHECK(back.has_value());
		CHECK_EQUAL(two, *back);
	}

	TEST(ImagesLoadThroughTheUiLoaderAndScaleToTheWidth) {
		FakeImageLoader loader;
		loader.sizes["docs/wide.png"] = { 2000, 500 };
		ViewScene scene("Intro\n\n![Wide shot](wide.png)\n");
		scene.view.setBaseDirectory("docs");
		UiDriver driver(scene.root, { 800, 600 });
		driver.ui().setImageLoader(&loader);
		scene.view.setMarkdown("Intro\n\n![Wide shot](wide.png \"title\")\n");
		driver.settle();

		const std::vector<MarkdownImage*> images = scene.view.images();
		CHECK_EQUAL(1u, images.size());
		CHECK_EQUAL("docs/wide.png", images[0]->path());
		CHECK_EQUAL("Wide shot", images[0]->alt());
		CHECK(images[0]->isLoaded());
		CHECK_CLOSE(800 * 0.25f, images[0]->size().y, 1.0f);

		bool painted = false;
		for (const PaintCommand& command : driver.paint().commands())
			painted |= command.kind == PaintCommand::Kind::Image && command.rect.size.x <= 800.5f;
		CHECK(painted);
		CHECK(!driver.paint().hasText("Wide shot"));
	}

	TEST(SmallImagesKeepTheirSizeAndInlineImagesKeepTheText) {
		FakeImageLoader loader;
		loader.sizes["icon.png"] = { 32, 24 };
		ViewScene scene("");
		UiDriver driver(scene.root, { 800, 600 });
		driver.ui().setImageLoader(&loader);
		scene.view.setMarkdown("see ![icon](icon.png) here");
		driver.settle();

		CHECK(driver.paint().hasText("see "));
		CHECK_EQUAL(1u, scene.view.images().size());
		CHECK_CLOSE(24, scene.view.images()[0]->size().y, 0.5f);
	}

	TEST(MissingImagesShowAPlaceholderWithTheAltText) {
		FakeImageLoader loader;
		ViewScene scene("");
		UiDriver driver(scene.root, { 800, 600 });
		driver.ui().setImageLoader(&loader);
		scene.view.setMarkdown("![Diagram](missing.png)");
		driver.settle();

		CHECK(!scene.view.images()[0]->isLoaded());
		CHECK(driver.paint().hasText("Diagram \u2014 not found"));
	}

	TEST(AbsoluteAndRemoteImagePathsAreNotRebased) {
		ViewScene scene("");
		scene.view.setBaseDirectory("/home/me/notes");
		CHECK_EQUAL("/tmp/a.png", scene.view.resolveImagePath("/tmp/a.png"));
		CHECK_EQUAL("https://x.y/a.png", scene.view.resolveImagePath("https://x.y/a.png"));
		CHECK_EQUAL("/home/me/notes/img/a.png", scene.view.resolveImagePath("img/a.png"));
	}

	TEST(ReplacingTheMarkdownRebuildsWithoutLeaks) {
		ViewScene scene("# a\n\n- [ ] x\n");
		UiDriver driver(scene.root);

		for (int round = 0; round < 20; round++) {
			scene.view.setMarkdown(
				"# round " + std::to_string(round) + "\n\n- [ ] item\n\n| a | b |\n|---|---|\n| 1 | 2 |\n"
			);
			driver.settle();
		}

		CHECK(driver.paint().hasText("round 19"));
		CHECK(driver.paint().isBalanced());
	}
}

namespace {
	int sectionAtPreviewTop(EditorScene& scene) {
		const halnf top = scene.editor.previewScroll().offset() - scene.editor.preview().rect().pos.y + 12;
		const std::optional<size_t> source = scene.editor.preview().sourceAt(std::max(0.0f, top) + 1);
		return source ? sectionOf(scene.editor.markdown(), *source) : -1;
	}
}

SUITE(MarkdownEditorWidget) {
	TEST(SplitModeShowsSourceAndPreviewSideBySide) {
		EditorScene scene("# Title\n");
		UiDriver driver(scene.root, { 1000, 600 });

		CHECK(scene.editor.mode() == MarkdownEditor::Mode::Split);
		CHECK(scene.editor.showsSource());
		CHECK(scene.editor.showsPreview());
		CHECK(scene.editor.sourceArea().windowRect().pos.x < scene.editor.previewScroll().windowRect().pos.x);
		CHECK(scene.editor.modeButton(MarkdownEditor::Mode::Split).isSelected());
	}

	TEST(ModeButtonsSwitchBetweenEditorSplitAndPreview) {
		EditorScene scene("# Title\n");
		UiDriver driver(scene.root, { 1000, 600 });

		driver.click(scene.editor.modeButton(MarkdownEditor::Mode::Preview));
		driver.settle();
		CHECK(scene.editor.mode() == MarkdownEditor::Mode::Preview);
		CHECK(!scene.editor.showsSource());
		CHECK(scene.editor.previewScroll().size().x > 900);

		driver.click(scene.editor.modeButton(MarkdownEditor::Mode::Editor));
		driver.settle();
		CHECK(scene.editor.mode() == MarkdownEditor::Mode::Editor);
		CHECK(!scene.editor.showsPreview());
		CHECK(scene.editor.sourceArea().size().x > 900);
	}

	TEST(TypingUpdatesThePreviewAfterAShortPause) {
		EditorScene scene("");
		UiDriver driver(scene.root, { 1000, 600 });

		driver.click(scene.editor.sourceArea().windowPosition() + Vec2F(20.0f, 20.0f));
		driver.type("# Live");
		driver.settle();

		CHECK_EQUAL("# Live", scene.editor.markdown());
		CHECK_EQUAL("# Live", scene.editor.preview().markdown());
		CHECK(driver.paint().findText("Live")->face == FontFace::Bold);
		CHECK(!scene.changes.empty());
	}

	TEST(SourceIsHighlightedByMarkdownRole) {
		EditorScene scene("# Head\n\ntext with `code` and [a link](x)\n");
		UiDriver driver(scene.root, { 1000, 600 });

		const std::vector<HighlightSpan>& spans = scene.editor.sourceArea().editor().highlights();
		const std::string& source = scene.editor.markdown();
		const auto covered = [&](const std::string& fragment) {
			const size_t at = source.find(fragment);
			for (const HighlightSpan& span : spans) {
				if (span.start <= at && span.end >= at + fragment.size()) return true;
			}
			return false;
		};
		CHECK(covered("# Head"));
		CHECK(covered("`code`"));
		CHECK(covered("a link](x)"));
		CHECK(!covered("text with"));
		CHECK(scene.editor.sourceArea().editor().fontFace() == FontFace::Monospace);
	}

	TEST(ToggledTasksInThePreviewUpdateTheSource) {
		EditorScene scene("- [ ] ship it\n");
		UiDriver driver(scene.root, { 1000, 600 });

		driver.click(*scene.editor.preview().taskBoxes().front());
		driver.settle();

		CHECK_EQUAL("- [x] ship it\n", scene.editor.markdown());
		CHECK_EQUAL("- [x] ship it\n", scene.changes.back());
	}

	TEST(ClickingPreviewTextMovesTheCaretToItsSource) {
		const std::string source = "# Title\n\nfirst paragraph\n\nsecond paragraph\n";
		EditorScene scene(source);
		UiDriver driver(scene.root, { 1000, 600 });

		const PaintCommand* second = textStartingWith(driver.paint(), "second");
		CHECK(second);
		driver.click(second->rect.center());

		CHECK(scene.editor.sourceArea().isEditing());
		CHECK_EQUAL(source.find("second"), scene.editor.sourceArea().editor().buffer().caret());
	}

	TEST(ScrollingTheSourceScrollsThePreviewToTheSameSection) {
		const std::string source = longDocument();
		EditorScene scene(source);
		UiDriver driver(scene.root, { 1000, 400 });

		driver.moveTo(scene.editor.sourceArea().windowPosition() + Vec2F(40.0f, 60.0f));
		for (int step = 0; step < 12; step++)
			driver.wheel(-1);
		driver.settle();

		const size_t topSource =
			scene.editor.sourceArea().editor().positionAtTop(scene.editor.sourceArea().scroll().offset());
		CHECK(topSource > 0);
		CHECK(scene.editor.previewScroll().offset() > 100);
		CHECK(std::abs(sectionAtPreviewTop(scene) - sectionOf(source, topSource)) <= 1);
	}

	TEST(ThePreviewFollowsTheSourceSmoothlyWhileItAnimates) {
		EditorScene scene(longDocument());
		UiDriver driver(scene.root, { 1000, 400 });
		driver.setAutoSettle(false);

		driver.moveTo(scene.editor.sourceArea().windowPosition() + Vec2F(40.0f, 60.0f));
		driver.wheel(-6);

		std::vector<halnf> previewOffsets;
		for (int frame = 0; frame < 40; frame++) {
			driver.advance(16);
			previewOffsets.push_back(scene.editor.previewScroll().currentOffset());
		}

		halnf largestStep = 0;
		size_t movingFrames = 0;
		for (size_t index = 1; index < previewOffsets.size(); index++) {
			const halnf step = previewOffsets[index] - previewOffsets[index - 1];
			largestStep = std::max(largestStep, step);
			movingFrames += step > 0.5f;
			CHECK(step >= -0.5f);
		}
		CHECK(previewOffsets.back() > 100);
		CHECK(movingFrames >= 4);
		CHECK(largestStep < previewOffsets.back() * 0.6f);
	}

	TEST(ScrollingThePreviewScrollsTheSourceToTheSameSection) {
		const std::string source = longDocument();
		EditorScene scene(source);
		UiDriver driver(scene.root, { 1000, 400 });

		driver.moveTo(scene.editor.previewScroll().windowPosition() + Vec2F(40.0f, 60.0f));
		for (int step = 0; step < 12; step++)
			driver.wheel(-1);
		driver.settle();

		const size_t topSource =
			scene.editor.sourceArea().editor().positionAtTop(scene.editor.sourceArea().scroll().offset());
		CHECK(scene.editor.sourceArea().scroll().offset() > 100);
		CHECK(std::abs(sectionAtPreviewTop(scene) - sectionOf(source, topSource)) <= 1);
	}

	TEST(PlacingTheCaretAloneDoesNotMoveThePreview) {
		const std::string source = longDocument();
		EditorScene scene(source);
		UiDriver driver(scene.root, { 1000, 400 });

		driver.click(scene.editor.sourceArea().windowPosition() + Vec2F(20.0f, 20.0f));
		scene.editor.sourceArea().editor().setCaret(source.find("Section 1\n"));
		driver.settle();

		CHECK_CLOSE(0, scene.editor.previewScroll().offset(), 0.5f);
	}

	TEST(ControlWheelZoomsBothPanesTogether) {
		EditorScene scene("# Title\n\nbody text\n");
		UiDriver driver(scene.root, { 1000, 600 });
		const halnf bodyBefore = driver.paint().findText("body text")->fontSize;
		const halnf lineBefore = scene.editor.sourceArea().editor().lineHeight();

		Modifiers control;
		control.control = true;
		driver.moveTo(scene.editor.previewScroll().windowPosition() + Vec2F(40.0f, 40.0f));
		for (int step = 0; step < 2; step++) {
			driver.ui().dispatch(
				Event::wheel(scene.editor.previewScroll().windowPosition() + Vec2F(40.0f, 40.0f), 1).withModifiers(control)
			);
		}
		driver.settle();

		CHECK(scene.editor.fontScale() > MarkdownEditor::kDefaultFontScale + 0.15f);
		CHECK(driver.paint().findText("body text")->fontSize > bodyBefore);
		CHECK(scene.editor.sourceArea().editor().lineHeight() > lineBefore);
		CHECK_CLOSE(0, scene.editor.previewScroll().offset(), 0.5f);

		scene.editor.setFontScale(100);
		CHECK_CLOSE(2.5f, scene.editor.fontScale(), 0.001f);
		scene.editor.setFontScale(0);
		CHECK_CLOSE(0.6f, scene.editor.fontScale(), 0.001f);
	}

	TEST(PastingAnImageSavesItAndInsertsALink) {
		EditorScene scene("Intro\n");
		FakeWindowHost host;
		std::vector<ClipboardImage> stored;
		scene.editor.setImageStore([&](const ClipboardImage& image) -> std::optional<std::string> {
			stored.push_back(image);
			return std::string("images/pasted-1.png");
		});
		UiDriver driver(scene.root, { 1000, 600 });
		driver.ui().setWindowHost(&host);
		host.image = { "image/png", std::string("\x89PNG fake", 9) };

		driver.click(scene.editor.sourceArea().windowPosition() + Vec2F(20.0f, 20.0f));
		scene.editor.sourceArea().editor().setCaret(scene.editor.markdown().size());
		driver.key(InputID::V, control());
		driver.settle();

		CHECK_EQUAL(1u, stored.size());
		CHECK_EQUAL("image/png", stored[0].mimeType);
		CHECK_EQUAL("Intro\n\n![](images/pasted-1.png)", scene.editor.markdown());
		CHECK_EQUAL(1u, scene.editor.preview().images().size());
		CHECK_EQUAL("images/pasted-1.png", scene.editor.preview().images()[0]->path());
	}

	TEST(WithoutAStoreAPastedImageIsSavedInTheApplicationImagesFolder) {
		EditorScene scene("Intro\n");
		FakeWindowHost host;
		const std::filesystem::path root =
			std::filesystem::temp_directory_path() / ("markdown-paste-" + std::to_string(::getpid()));
		std::filesystem::remove_all(root);
		host.storage = AppStorage(root.string());
		UiDriver driver(scene.root, { 1000, 600 });
		driver.ui().setWindowHost(&host);
		host.image = { "image/png", std::string("\x89PNG fake", 9) };

		driver.click(scene.editor.sourceArea().windowPosition() + Vec2F(20.0f, 20.0f));
		scene.editor.sourceArea().editor().setCaret(scene.editor.markdown().size());
		driver.key(InputID::V, control());
		driver.settle();

		const std::filesystem::path saved = root / "images" / "pasted-1.png";
		CHECK(std::filesystem::exists(saved));
		CHECK_EQUAL("Intro\n\n![](" + saved.string() + ")", scene.editor.markdown());
		CHECK(scene.editor.statusLabel().text().find("Pasted image saved") != std::string::npos);

		driver.key(InputID::V, control());
		CHECK(std::filesystem::exists(root / "images" / "pasted-2.png"));
		std::filesystem::remove_all(root);
	}

	TEST(PastedImagesBecomeTheirOwnParagraph) {
		const std::string path = "a.png";
		CHECK_EQUAL("\n\n![](a.png)\n\n", imageParagraph("# Head", 1, 1, path));
		CHECK_EQUAL("![](a.png)\n\n", imageParagraph("# Head", 0, 0, path));
		CHECK_EQUAL("\n![](a.png)\n", imageParagraph("one\n\ntwo", 4, 4, path));
		CHECK_EQUAL("![](a.png)", imageParagraph("one\n\n\n\ntwo", 5, 5, path));
		CHECK_EQUAL("\n\n![](a.png)", imageParagraph("text", 4, 4, path));
	}

	TEST(PastingTextStillPastesTextWhenTheClipboardHasNoImage) {
		EditorScene scene("");
		FakeWindowHost host;
		bool storeCalled = false;
		scene.editor.setImageStore([&](const ClipboardImage&) -> std::optional<std::string> {
			storeCalled = true;
			return std::nullopt;
		});
		UiDriver driver(scene.root, { 1000, 600 });
		driver.ui().setWindowHost(&host);
		host.clipboard = "plain words";

		driver.click(scene.editor.sourceArea().windowPosition() + Vec2F(20.0f, 20.0f));
		driver.key(InputID::V, control());

		CHECK(!storeCalled);
		CHECK_EQUAL("plain words", scene.editor.markdown());
	}

	TEST(AFailedImageSaveLeavesTheTextUntouched) {
		EditorScene scene("keep");
		FakeWindowHost host;
		scene.editor.setImageStore([](const ClipboardImage&) -> std::optional<std::string> { return std::nullopt; });
		UiDriver driver(scene.root, { 1000, 600 });
		driver.ui().setWindowHost(&host);
		host.image = { "image/png", "bytes" };
		host.clipboard = "should not appear";

		driver.click(scene.editor.sourceArea().windowPosition() + Vec2F(20.0f, 20.0f));
		driver.key(InputID::V, control());

		CHECK_EQUAL("keep", scene.editor.markdown());
		CHECK_EQUAL("Could not save the pasted image", scene.editor.statusLabel().text());
	}

	TEST(BothTextAreasUseTheWindowColorButTheEditorItselfDoesNot) {
		EditorScene scene("# Title\n\nbody\n");
		UiDriver driver(scene.root, { 1000, 600 });
		const RGBA window = driver.ui().theme().palette.window;
		const RecordingPainter& painted = driver.paint();

		const auto filledWithWindow = [&](const RectF& area) {
			for (const PaintCommand& command : painted.commands()) {
				if (command.kind != PaintCommand::Kind::FillRect || !(command.color == window)) continue;
				if (std::abs(command.rect.pos.x - area.pos.x) < 1 && std::abs(command.rect.pos.y - area.pos.y) < 1 &&
						std::abs(command.rect.size.x - area.size.x) < 1 && std::abs(command.rect.size.y - area.size.y) < 1)
					return true;
			}
			return false;
		};
		CHECK(filledWithWindow(scene.editor.sourceArea().windowRect()));
		CHECK(filledWithWindow(scene.editor.previewScroll().parent()->windowRect()));
		CHECK(!filledWithWindow(scene.editor.windowRect()));
	}

	TEST(TextStartsAt130PercentWithoutShowingTheDefaultZoom) {
		EditorScene scene("body text\n");
		UiDriver driver(scene.root, { 1000, 600 });
		CHECK_CLOSE(1.3f, scene.editor.fontScale(), 0.001f);
		CHECK(scene.editor.statusLabel().text().find('%') == std::string::npos);
		CHECK(driver.paint().findText("body text")->fontSize > driver.ui().theme().metrics.fontSize * 1.25f);

		scene.editor.zoomBy(1);
		CHECK(scene.editor.statusLabel().text().find("140%") != std::string::npos);
	}

	TEST(StatusCountsLinesAndWords) {
		EditorScene scene("one two\nthree");
		UiDriver driver(scene.root, { 1000, 600 });

		CHECK_EQUAL("2 lines, 3 words", scene.editor.statusLabel().text());
	}

	TEST(ClickingAPreviewLinkOpensItWithTheDesktop) {
		EditorScene scene("go to [site](https://example.com) now\n");
		FakeWindowHost host;
		UiDriver driver(scene.root, { 1000, 600 });
		driver.ui().setWindowHost(&host);

		const PaintCommand* site = driver.paint().findText("site");
		CHECK(site);
		driver.click(site->rect.center());

		CHECK_EQUAL(1u, host.openedUrls.size());
		CHECK_EQUAL("https://example.com", host.openedUrls[0]);
		CHECK_EQUAL("Opened https://example.com", scene.editor.statusLabel().text());
	}

	TEST(ControlClickingALinkInTheSourceOpensIt) {
		EditorScene scene("[site](https://example.com)\n");
		FakeWindowHost host;
		UiDriver driver(scene.root, { 1000, 600 });
		driver.ui().setWindowHost(&host);
		TextAreaEditor& editor = scene.editor.sourceArea().editor();
		const Vec2F onLink = editor.windowPosition() + Vec2F(30.0f, editor.textTop() + editor.lineHeight() / 2);

		driver.click(onLink);
		CHECK(host.openedUrls.empty());

		driver.click(onLink, control());
		CHECK_EQUAL(1u, host.openedUrls.size());
		CHECK_EQUAL("https://example.com", host.openedUrls[0]);
	}

	TEST(RelativeLinksOpenAsFilesBesideTheDocument) {
		EditorScene scene("[notes](other.md#part)\n");
		FakeWindowHost host;
		scene.editor.setBaseDirectory("/home/me/docs");
		UiDriver driver(scene.root, { 1000, 600 });
		driver.ui().setWindowHost(&host);

		scene.editor.openLink("other.md#part");
		scene.editor.openLink("../up.png");

		CHECK_EQUAL(2u, host.openedUrls.size());
		CHECK_EQUAL("/home/me/docs/other.md", host.openedUrls[0]);
		CHECK_EQUAL("/home/me/up.png", host.openedUrls[1]);
		CHECK_EQUAL("mailto:a@b.c", scene.editor.preview().resolveExternalLink("mailto:a@b.c"));
		CHECK_EQUAL("file:///tmp/a.png", scene.editor.preview().resolveExternalLink("file:///tmp/a.png"));
		CHECK_EQUAL("/home/me/docs/my notes.md", scene.editor.preview().resolveExternalLink("my%20notes.md"));
		CHECK_EQUAL("/home/me/docs/my notes.md", scene.editor.preview().resolveExternalLink("my notes.md?x=1"));
		CHECK_EQUAL("/tmp/abs.txt", scene.editor.preview().resolveExternalLink("/tmp/abs.txt"));
	}

	TEST(AnchorLinksScrollToTheirHeadingInsteadOfOpening) {
		std::string source = "[jump](#far-away)\n\n";
		for (int line = 0; line < 80; line++)
			source += "filler paragraph " + std::to_string(line) + "\n\n";
		source += "## Far away\n\nend\n";
		EditorScene scene(source);
		FakeWindowHost host;
		UiDriver driver(scene.root, { 1000, 400 });
		driver.ui().setWindowHost(&host);

		CHECK(scene.editor.openLink("#far-away"));
		driver.settle();

		CHECK(host.openedUrls.empty());
		CHECK(scene.editor.previewScroll().offset() > 100);
		CHECK_EQUAL("Far away", scene.editor.statusLabel().text());
		CHECK(!scene.editor.openLink("#nowhere"));
	}

	TEST(ALinkHandlerCanTakeOverOpening) {
		EditorScene scene("x\n");
		FakeWindowHost host;
		std::vector<std::string> handled;
		scene.editor.setLinkHandler([&](const std::string& link) {
			handled.push_back(link);
			return link.starts_with("app:");
		});
		UiDriver driver(scene.root, { 1000, 600 });
		driver.ui().setWindowHost(&host);

		scene.editor.openLink("app:settings");
		scene.editor.openLink("https://example.com");

		CHECK_EQUAL(2u, handled.size());
		CHECK_EQUAL(1u, host.openedUrls.size());
		CHECK_EQUAL("https://example.com", host.openedUrls[0]);
	}
}

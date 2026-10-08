#include "MarkdownEditor.hpp"

#include "BoxLayout.hpp"
#include "StackLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"
#include "WindowHost.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::markdown;

namespace {
	constexpr halnf kRenderDelayMs = 150;
	constexpr halnf kPreviewTopMargin = 12;
	constexpr halnf kZoomStep = 0.1f;
	constexpr halnf kMinimumFontScale = 0.6f;
	constexpr halnf kMaximumFontScale = 2.5f;

	struct EditorPalette {
		RGBA heading;
		RGBA marker;
		RGBA code;
		RGBA link;
		RGBA emphasis;
		RGBA quote;
	};

	EditorPalette editorPalette(const Palette& palette) {
		return {
			palette.accentHovered,
			palette.mutedText,
			Palette::mix(palette.warningText, palette.text, 0.35f),
			Palette::mix(palette.accent, palette.text, 0.35f),
			{ 0.80f, 0.62f, 0.86f, 1.0f },
			palette.mutedText,
		};
	}

	size_t lineEndAt(const std::string& text, size_t offset) {
		const size_t end = text.find('\n', offset);
		return end == std::string::npos ? text.size() : end;
	}

	class SourceHighlighter {
	public:
		SourceHighlighter(const std::string& source, const Palette& palette) :
			mSource(source),
			mColors(editorPalette(palette)) {}

		std::vector<HighlightSpan> run() {
			highlightLineMarkers();
			highlightFromDocument(MarkdownDocument(mSource).blocks());
			return std::move(mSpans);
		}

	private:
		void add(size_t start, size_t end, const RGBA& color) {
			if (end > start && end <= mSource.size()) mSpans.push_back({ start, end, color });
		}

		void highlightLineMarkers() {
			size_t lineStart = 0;
			while (lineStart < mSource.size()) {
				const size_t lineEnd = lineEndAt(mSource, lineStart);
				size_t marker = lineStart;
				while (marker < lineEnd && mSource[marker] == ' ')
					marker++;

				if (marker < lineEnd && mSource[marker] == '#') {
					add(lineStart, lineEnd, mColors.heading);
				} else if (marker < lineEnd && mSource[marker] == '>') {
					add(lineStart, lineEnd, mColors.quote);
				} else if (mSource.compare(marker, 3, "```") == 0 || mSource.compare(marker, 3, "~~~") == 0) {
					add(lineStart, lineEnd, mColors.marker);
				} else if (
					marker + 1 < lineEnd && (mSource[marker] == '-' || mSource[marker] == '*' || mSource[marker] == '+') &&
					mSource[marker + 1] == ' '
				) {
					add(marker, marker + 1, mColors.marker);
				}
				lineStart = lineEnd + 1;
			}
		}

		void highlightFromDocument(const std::vector<Block>& blocks) {
			for (const Block& block : blocks) {
				if (block.kind == BlockKind::Code && !block.source.isEmpty())
					add(block.source.begin, block.source.end, mColors.code);
				if (block.kind == BlockKind::Rule) add(block.source.begin, block.source.end, mColors.marker);
				if (block.kind != BlockKind::Heading) highlightRuns(block);
				highlightFromDocument(block.children);
			}
		}

		void highlightRuns(const Block& block) {
			for (const TextRun& run : block.runs) {
				if (run.source.isEmpty()) continue;
				if (run.style.code) {
					add(widenTo(run.source.begin, '`', -1), widenTo(run.source.end, '`', 1), mColors.code);
				} else if (!run.link.empty()) {
					add(run.source.begin, linkEnd(run.source.end), mColors.link);
				} else if (run.style.strong || run.style.emphasis || run.style.strikethrough) {
					add(
						widenTo(run.source.begin, '*', -1, '_', '~'), widenTo(run.source.end, '*', 1, '_', '~'), mColors.emphasis
					);
				}
			}
		}

		[[nodiscard]] size_t widenTo(size_t offset, char mark, int direction, char alternative = 0, char third = 0) const {
			const auto matches = [&](char character) {
				return character == mark || (alternative && character == alternative) || (third && character == third);
			};
			if (direction < 0) {
				while (offset > 0 && matches(mSource[offset - 1]))
					offset--;
			} else {
				while (offset < mSource.size() && matches(mSource[offset]))
					offset++;
			}
			return offset;
		}

		[[nodiscard]] size_t linkEnd(size_t offset) const {
			if (
				offset < mSource.size() && mSource[offset] == ']' && offset + 1 < mSource.size() && mSource[offset + 1] == '('
			) {
				const size_t close = mSource.find(')', offset);
				if (close != std::string::npos && close < lineEndAt(mSource, offset)) return close + 1;
			}
			return offset;
		}

		const std::string& mSource;
		EditorPalette mColors;
		std::vector<HighlightSpan> mSpans;
	};
}

std::string tp::ui::imageParagraph(const std::string& source, size_t start, size_t end, const std::string& path) {
	const auto newlinesBefore = [&] {
		size_t count = 0;
		for (size_t at = start; at > 0 && source[at - 1] == '\n' && count < 2; at--)
			count++;
		return start == 0 ? 2 : count;
	};
	const auto newlinesAfter = [&] {
		size_t count = 0;
		for (size_t at = end; at < source.size() && source[at] == '\n' && count < 2; at++)
			count++;
		return end >= source.size() ? 2 : count;
	};

	return std::string(2 - newlinesBefore(), '\n') + "![](" + path + ")" + std::string(2 - newlinesAfter(), '\n');
}

std::vector<HighlightSpan> tp::ui::highlightMarkdown(const std::string& source, const Palette& palette) {
	return SourceHighlighter(source, palette).run();
}

MarkdownEditor::MarkdownEditor() {
	setDebugName("markdown editor");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	emplaceLayout<ColumnLayout>(6);

	mToolbar.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mToolbar.emplaceLayout<RowLayout>(4).setCrossAlign(CrossAlign::Center);
	for (Button* button : { &mEditorButton, &mSplitButton, &mPreviewButton }) {
		button->setStyle(Button::Style::Tab);
		mToolbar.addChild(*button);
	}
	mEditorButton.setTooltip("Show only the source");
	mSplitButton.setTooltip("Source and preview side by side");
	mPreviewButton.setTooltip("Show only the rendered document");
	mEditorButton.setOnActivate([this] { setMode(Mode::Editor); });
	mSplitButton.setOnActivate([this] { setMode(Mode::Split); });
	mPreviewButton.setOnActivate([this] { setMode(Mode::Preview); });
	mToolbarSpacer.setSizing(AxisSizing::fill(), AxisSizing::fixed(1));
	mToolbar.addChild(mToolbarSpacer);
	mStatus.setTone(Label::Tone::Muted);
	mToolbar.addChild(mStatus);

	mSourcePane.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mSourcePane.emplaceLayout<StackLayout>();
	mSourceArea.setFillsHeight(true);
	mSourceArea.setFontFace(FontFace::Monospace);
	mSourceArea.setFillsWithWindowColor(true);
	mSourceArea.setPlaceholder("Write markdown here");
	mSourceArea.setHighlighter([this](const std::string& source) { return highlightMarkdown(source, theme().palette); });
	mSourceArea.setOnEdited([this](const std::string& source) { sourceEdited(source); });
	mSourceArea.scroll().setOnScrolled([this](halnf) { scrolled(Pane::Source); });
	mSourceArea.editor().setPasteHandler([this] {
		WindowHost* host = ui() ? ui()->windowHost() : nullptr;
		return host && pasteImage(host->clipboardImage());
	});
	mSourcePane.addChild(mSourceArea);

	mPreviewPane.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mPreviewPane.setCornerRadius(theme().metrics.cornerRadius);
	mPreviewPane.emplaceLayout<StackLayout>();
	mPreviewContent.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mPreviewContent.emplaceLayout<ColumnLayout>().setPadding({ 18, kPreviewTopMargin, 18, 24 });
	mPreviewContent.addChild(mPreview);
	mPreviewScroll.setContent(&mPreviewContent);
	mPreviewScroll.setWheelStep(theme().metrics.lineHeight * 3);
	mPreviewScroll.setOnScrolled([this](halnf) { scrolled(Pane::Preview); });
	mPreviewPane.addChild(mPreviewScroll);

	mPreview.setOnTaskToggled([this](const std::string& source) {
		mSourceArea.setText(source);
		refreshStatus();
		if (mOnChanged) mOnChanged(source);
	});
	mPreview.setOnSourceActivated([this](size_t offset) { revealSourceInEditor(offset); });
	mPreview.setOnLinkActivated([this](const std::string& link) { openLink(link); });

	mSplit.setFirst(&mSourcePane);
	mSplit.setSecond(&mPreviewPane);
	mSplit.setMinimumPaneLength(160);

	addChild(mSplit);
	addChild(mToolbar);
	applyMode();
	setFontScale(kDefaultFontScale);
	refreshStatus();
}

Button& MarkdownEditor::modeButton(Mode mode) {
	switch (mode) {
		case Mode::Editor: return mEditorButton;
		case Mode::Preview: return mPreviewButton;
		default: return mSplitButton;
	}
}

void MarkdownEditor::setMarkdown(std::string source) {
	mSourceArea.setText(source);
	mPreview.setMarkdown(std::move(source));
	mPendingRenderMs = -1;
	refreshStatus();
}

void MarkdownEditor::setMode(Mode mode) {
	if (mode == mMode) return;
	mMode = mode;
	applyMode();
}

void MarkdownEditor::applyMode() {
	mEditorButton.setSelected(mMode == Mode::Editor);
	mSplitButton.setSelected(mMode == Mode::Split);
	mPreviewButton.setSelected(mMode == Mode::Preview);

	mSourcePane.setVisible(mMode != Mode::Preview);
	mPreviewPane.setVisible(mMode != Mode::Editor);
	mSplit.handle().setVisible(mMode == Mode::Split);
	if (mMode == Mode::Editor) mSplit.setRatio(1);
	if (mMode == Mode::Preview) mSplit.setRatio(0);
	if (mMode == Mode::Split) mSplit.setRatio(0.5f);
	requestLayout();
}

void MarkdownEditor::sourceEdited(const std::string& source) {
	mPendingRenderMs = kRenderDelayMs;
	startTicking();
	refreshStatus();
	if (mOnChanged) mOnChanged(source);
}

void MarkdownEditor::onTick(halnf deltaMs) {
	if (mPendingRenderMs >= 0) {
		mPendingRenderMs -= deltaMs;
		if (mPendingRenderMs > 0) return;
		mPendingRenderMs = -1;
		mPreview.setMarkdown(mSourceArea.text());
	}

	stopTicking();
}

void MarkdownEditor::scrolled(Pane leader) {
	if (mMode != Mode::Split || mFollowing) return;

	mFollowing = true;
	follow(leader);
	mFollowing = false;
}

halnf MarkdownEditor::previewContentTop() const { return mPreview.rect().pos.y - kPreviewTopMargin; }

std::vector<MarkdownEditor::ScrollAnchor> MarkdownEditor::scrollAnchors() const {
	const TextAreaEditor& editor = mSourceArea.editor();
	std::vector<ScrollAnchor> anchors = { { 0, 0 } };

	for (const MarkdownBlockWidget* block : mPreview.topBlocks()) {
		const halnf source = editor.topOfLineContaining(block->source().begin) - editor.textTop();
		const halnf preview = previewContentTop() + block->rect().pos.y;
		if (source <= anchors.back().source || preview <= anchors.back().preview) continue;
		anchors.push_back({ source, preview });
	}

	return anchors;
}

namespace {
	using Anchor = MarkdownEditor::ScrollAnchor;

	halnf mapThroughAnchors(const std::vector<Anchor>& anchors, halnf value, halnf Anchor::* from, halnf Anchor::* to) {
		if (anchors.empty()) return 0;
		if (value <= anchors.front().*from) return anchors.front().*to;

		for (size_t index = 1; index < anchors.size(); index++) {
			const Anchor& low = anchors[index - 1];
			const Anchor& high = anchors[index];
			if (value > high.*from) continue;

			const halnf span = high.*from - low.*from;
			const halnf fraction = span > 0 ? (value - low.*from) / span : 0;
			return low.*to + (high.*to - low.*to) * fraction;
		}
		return anchors.back().*to;
	}

	halnf followerOffset(
		const std::vector<Anchor>& anchors,
		halnf leaderOffset,
		halnf leaderEnd,
		halnf leaderViewport,
		halnf followerEnd,
		halnf Anchor::* leader,
		halnf Anchor::* follower
	) {
		const halnf aligned = mapThroughAnchors(anchors, leaderOffset, leader, follower);
		const halnf tailStart = std::max(0.0f, leaderEnd - leaderViewport);
		if (leaderEnd <= 0 || leaderOffset <= tailStart) return std::clamp(aligned, 0.0f, followerEnd);

		const halnf tail = std::clamp((leaderOffset - tailStart) / std::max(1.0f, leaderEnd - tailStart), 0.0f, 1.0f);
		const halnf tailFrom = mapThroughAnchors(anchors, tailStart, leader, follower);
		const halnf towardsEnd = tailFrom + (followerEnd - tailFrom) * tail;
		return std::clamp(aligned + (towardsEnd - aligned) * tail, 0.0f, followerEnd);
	}
}

void MarkdownEditor::follow(Pane leader) {
	if (ui()) ui()->updateLayout();
	const std::vector<ScrollAnchor> anchors = scrollAnchors();
	ScrollView& source = mSourceArea.scroll();

	if (leader == Pane::Source) {
		mPreviewScroll.jumpTo(followerOffset(
			anchors,
			source.currentOffset(),
			source.maximumOffset(),
			source.viewportLength(),
			mPreviewScroll.maximumOffset(),
			&ScrollAnchor::source,
			&ScrollAnchor::preview
		));
		return;
	}
	source.jumpTo(followerOffset(
		anchors,
		mPreviewScroll.currentOffset(),
		mPreviewScroll.maximumOffset(),
		mPreviewScroll.viewportLength(),
		source.maximumOffset(),
		&ScrollAnchor::preview,
		&ScrollAnchor::source
	));
}

size_t MarkdownEditor::sourceAtTopOfEditor() const {
	return mSourceArea.editor().positionAtTop(mSourceArea.scroll().currentOffset());
}

void MarkdownEditor::syncPreviewToSource() {
	if (ui()) ui()->updateLayout();
	mFollowing = true;
	follow(Pane::Source);
	mFollowing = false;
}

void MarkdownEditor::syncSourceToPreview() {
	if (ui()) ui()->updateLayout();
	mFollowing = true;
	follow(Pane::Preview);
	mFollowing = false;
}

void MarkdownEditor::setFontScale(halnf scale) {
	scale = std::clamp(scale, kMinimumFontScale, kMaximumFontScale);
	if (scale == mFontScale) return;

	if (ui()) ui()->updateLayout();
	const bool previewLeads = mMode == Mode::Preview;
	const std::optional<size_t> previewAnchor = mPreview.sourceAt(mPreviewScroll.currentOffset() - previewContentTop());
	const size_t anchor = previewLeads ? previewAnchor.value_or(0) : sourceAtTopOfEditor();
	mFontScale = scale;
	mSourceArea.setFontScale(scale);
	mPreview.setFontScale(scale);
	mPreviewScroll.setWheelStep(theme().metrics.lineHeight * 3 * scale);
	if (ui()) ui()->updateLayout();

	mFollowing = true;
	if (previewLeads) {
		const std::optional<halnf> top = mPreview.offsetOfSource(anchor);
		mPreviewScroll.jumpTo(top ? previewContentTop() + *top : 0);
	} else {
		mSourceArea.scroll().jumpTo(mSourceArea.editor().topOfLineContaining(anchor) - mSourceArea.editor().textTop());
	}
	mFollowing = false;
	if (previewLeads) {
		syncSourceToPreview();
	} else {
		syncPreviewToSource();
	}
	refreshStatus();
}

void MarkdownEditor::zoomBy(int steps) { setFontScale(std::round((mFontScale + kZoomStep * (halnf) steps) * 10) / 10); }

bool MarkdownEditor::opensLinkInSource(const Event& event) const {
	return event.type() == Event::Type::PointerDown && event.button() == PointerButton::Primary &&
				 event.modifiers().control && event.target() && mSourceArea.editor().isSelfOrAncestorOf(*event.target());
}

void MarkdownEditor::filterDescendantEvent(Event& event) {
	if (opensLinkInSource(event)) {
		TextAreaEditor& editor = mSourceArea.editor();
		const size_t position = editor.positionAt(editor.toLocal(event.windowPosition()));
		if (const std::optional<std::string> link = mPreview.document().linkAt(position)) {
			openLink(*link);
			event.accept();
		}
		return;
	}
	if (event.type() != Event::Type::Wheel || !event.modifiers().control || event.wheelAmount() == 0) return;

	zoomBy(event.wheelAmount() > 0 ? 1 : -1);
	event.accept();
}

bool MarkdownEditor::openLink(const std::string& link) {
	if (link.empty()) return false;
	if (mLinkHandler && mLinkHandler(link)) return true;
	if (link.front() == '#') return revealHeading(link);

	const std::string target = mPreview.resolveExternalLink(link);
	WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	const bool opened = host && host->openUrl(target);
	mStatus.setText(opened ? "Opened " + target : "Could not open " + target);
	return opened;
}

bool MarkdownEditor::revealHeading(const std::string& anchor) {
	const std::optional<markdown::HeadingEntry> heading = mPreview.document().headingForAnchor(anchor);
	if (!heading) {
		mStatus.setText("No heading " + anchor);
		return false;
	}

	if (ui()) ui()->updateLayout();
	if (mMode == Mode::Editor) {
		const TextAreaEditor& editor = mSourceArea.editor();
		mSourceArea.scroll().scrollTo(editor.topOfLineContaining(heading->sourceOffset) - editor.textTop());
	} else {
		const std::optional<halnf> top = mPreview.offsetOfSource(heading->sourceOffset);
		mPreviewScroll.scrollTo(top ? previewContentTop() + *top : 0);
	}
	mStatus.setText(heading->title);
	return true;
}

void MarkdownEditor::revealSourceInEditor(size_t sourceOffset) {
	if (mMode == Mode::Preview) return;
	if (ui()) ui()->setKeyboardFocus(&mSourceArea.editor());
	mSourceArea.editor().setCaret(std::min(sourceOffset, mSourceArea.text().size()));
}

std::optional<std::string> tp::ui::saveClipboardImage(const ClipboardImage& image, const std::string& directory) {
	std::error_code error;
	std::filesystem::create_directories(directory, error);
	if (!std::filesystem::is_directory(directory, error)) return std::nullopt;

	const size_t slash = image.mimeType.find('/');
	const std::string extension = slash == std::string::npos ? "png" : image.mimeType.substr(slash + 1);
	std::filesystem::path target;
	for (int index = 1;; index++) {
		target = std::filesystem::path(directory) / ("pasted-" + std::to_string(index) + "." + extension);
		if (!std::filesystem::exists(target, error)) break;
	}

	std::ofstream file(target, std::ios::binary | std::ios::trunc);
	if (!file || !file.write(image.bytes.data(), (std::streamsize) image.bytes.size())) return std::nullopt;
	return target.string();
}

std::optional<std::string> MarkdownEditor::storeImage(const ClipboardImage& image) const {
	if (mImageStore) return mImageStore(image);
	WindowHost* host = ui() ? ui()->windowHost() : nullptr;
	if (!host) return std::nullopt;
	return saveClipboardImage(image, host->persistentAppStorage().imagesDirectory());
}

bool MarkdownEditor::pasteImage(const ClipboardImage& image) {
	if (image.isEmpty()) return false;

	const std::optional<std::string> path = storeImage(image);
	if (!path) {
		mStatus.setText("Could not save the pasted image");
		return true;
	}

	mSourceArea.editor().insert(imageParagraph(
		mSourceArea.text(),
		mSourceArea.editor().buffer().selectionStart(),
		mSourceArea.editor().buffer().selectionEnd(),
		*path
	));
	mStatus.setText("Pasted image saved as " + *path);
	return true;
}

void MarkdownEditor::refreshStatus() {
	const std::string& source = mSourceArea.text();
	size_t words = 0;
	bool inWord = false;
	for (char character : source) {
		const bool space = character == ' ' || character == '\n' || character == '\t';
		if (!space && !inWord) words++;
		inWord = !space;
	}
	const size_t lines = source.empty() ? 0 : lineNumberAt(source, source.size()) + 1;
	std::string status = std::to_string(lines) + " lines, " + std::to_string(words) + " words";
	if (mFontScale != kDefaultFontScale) status += ", " + std::to_string((int) std::round(mFontScale * 100)) + "%";
	mStatus.setText(status);
}

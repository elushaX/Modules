#include "MarkdownView.hpp"

#include "BoxLayout.hpp"
#include "Event.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::markdown;

namespace {
	constexpr halnf kListGap = 4;
	constexpr halnf kTableGap = 1;
	constexpr halnf kMarkerGap = 6;
	constexpr halnf kMinimumFontScale = 0.5f;
	constexpr halnf kMaximumFontScale = 3.0f;
	constexpr halnf kMissingImageHeight = 64;

	RGBA codeChipColor(const Palette& palette) { return Palette::mix(palette.field, palette.raisedSurface, 0.8f); }

	RGBA highlightColor(const Palette& palette) { return Palette::mix(palette.warningText, palette.surface, 0.7f); }

	RGBA admonitionColor(const Palette& palette, const std::string& type) {
		if (type == "warning" || type == "caution") return palette.warningText;
		if (type == "tip") return { 0.36f, 0.72f, 0.45f, 1.0f };
		if (type == "important") return { 0.62f, 0.46f, 0.86f, 1.0f };
		return palette.accent;
	}

	FontFace faceFor(const InlineStyle& style, FontFace baseFace) {
		if (style.code) return FontFace::Monospace;

		const bool bold = style.strong || baseFace == FontFace::Bold || baseFace == FontFace::BoldItalic;
		const bool italic = style.emphasis || baseFace == FontFace::Italic || baseFace == FontFace::BoldItalic;
		if (bold && italic) return FontFace::BoldItalic;
		if (bold) return FontFace::Bold;
		if (italic) return FontFace::Italic;
		return FontFace::Regular;
	}

	TextAlign textAlignFor(CellAlign align) {
		switch (align) {
			case CellAlign::Center: return TextAlign::Center;
			case CellAlign::Right: return TextAlign::End;
			default: return TextAlign::Start;
		}
	}

	std::string titleCase(const std::string& word) {
		if (word.empty()) return word;
		std::string titled = word;
		titled[0] = (char) std::toupper((unsigned char) titled[0]);
		return titled;
	}

	class TableLayout : public Layout {
	public:
		explicit TableLayout(size_t columns) :
			mColumns(std::max<size_t>(columns, 1)) {}

	protected:
		[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
			if (axis == Axis::Horizontal) {
				halnf preferred = 0;
				halnf minimum = 0;
				for (size_t column = 0; column < mColumns; column++) {
					const Extent widest = widestIn(context, column);
					preferred += widest.preferred;
					minimum += widest.minimum;
				}
				const halnf gaps = kTableGap * (halnf) (mColumns - 1);
				return Extent::shrinkable(preferred + gaps, minimum + gaps);
			}

			const std::vector<halnf> widths = columnWidths(context, crossLength);
			halnf height = 0;
			for (size_t row = 0; row < rowCount(context); row++)
				height += rowHeight(context, row, widths) + (row > 0 ? kTableGap : 0);
			return Extent::natural(height);
		}

		void arrangeChildren(LayoutContext& context, const RectF& contentArea) override {
			const std::vector<halnf> widths = columnWidths(context, contentArea.size.x);
			halnf y = contentArea.pos.y;
			for (size_t row = 0; row < rowCount(context); row++) {
				const halnf height = rowHeight(context, row, widths);
				halnf x = contentArea.pos.x;
				for (size_t column = 0; column < mColumns; column++) {
					Widget* cell = cellAt(context, row, column);
					if (cell) context.place(*cell, makeRect(x, y, widths[column], height));
					x += widths[column] + kTableGap;
				}
				y += height + kTableGap;
			}
		}

	private:
		[[nodiscard]] size_t rowCount(const LayoutContext& context) const {
			return (context.children().size() + mColumns - 1) / mColumns;
		}

		[[nodiscard]] Widget* cellAt(const LayoutContext& context, size_t row, size_t column) const {
			const size_t index = row * mColumns + column;
			return index < context.children().size() ? context.children()[index] : nullptr;
		}

		[[nodiscard]] Extent widestIn(const LayoutContext& context, size_t column) const {
			Extent widest = { 0, 0, kUnbounded, 0 };
			for (size_t row = 0; row < rowCount(context); row++) {
				const Widget* cell = cellAt(context, row, column);
				if (!cell) continue;
				const Extent extent = context.measure(*cell, Axis::Horizontal, kUnbounded);
				widest.preferred = std::max(widest.preferred, extent.preferred);
				widest.minimum = std::max(widest.minimum, extent.minimum);
			}
			return widest;
		}

		[[nodiscard]] std::vector<halnf> columnWidths(const LayoutContext& context, halnf available) const {
			std::vector<halnf> preferred(mColumns);
			std::vector<halnf> minimum(mColumns);
			halnf preferredTotal = 0;
			halnf minimumTotal = 0;
			for (size_t column = 0; column < mColumns; column++) {
				const Extent extent = widestIn(context, column);
				preferred[column] = extent.preferred;
				minimum[column] = extent.minimum;
				preferredTotal += extent.preferred;
				minimumTotal += extent.minimum;
			}

			const halnf usable = available - kTableGap * (halnf) (mColumns - 1);
			if (available >= kUnbounded || usable >= preferredTotal) {
				const halnf extra = available >= kUnbounded ? 0 : (usable - preferredTotal) / (halnf) mColumns;
				for (halnf& width : preferred)
					width += extra;
				return preferred;
			}

			const halnf slack = preferredTotal - minimumTotal;
			const halnf share = slack > 0 ? std::max(0.0f, usable - minimumTotal) / slack : 0;
			std::vector<halnf> widths(mColumns);
			for (size_t column = 0; column < mColumns; column++)
				widths[column] = minimum[column] + (preferred[column] - minimum[column]) * share;
			return widths;
		}

		[[nodiscard]] halnf rowHeight(const LayoutContext& context, size_t row, const std::vector<halnf>& widths) const {
			halnf height = 0;
			for (size_t column = 0; column < mColumns; column++) {
				const Widget* cell = cellAt(context, row, column);
				if (cell) height = std::max(height, context.measure(*cell, Axis::Vertical, widths[column]).preferred);
			}
			return height;
		}

		size_t mColumns;
	};
}

MarkdownBlockWidget::MarkdownBlockWidget(const Block& block, Surface surface) :
	mKind(block.kind),
	mSurface(surface),
	mSource(block.source),
	mAdmonitionType(block.admonitionType) {
	setDebugName("markdown block");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
}

void MarkdownBlockWidget::paint(Painter& painter) {
	const Theme& style = theme();
	const Palette& palette = style.palette;
	const halnf radius = style.metrics.indicatorCornerRadius;

	if (mSurface == Surface::None) return;
	if (mSurface == Surface::TableGrid) {
		painter.fillRect(localRect(), palette.border, radius);
		return;
	}

	switch (mKind) {
		case BlockKind::Code:
		case BlockKind::Html: painter.fillRect(localRect(), codeChipColor(palette), radius); break;
		case BlockKind::Quote:
			painter.fillRect(makeRect(0, 0, MarkdownStyle().quoteBarWidth, size().y), palette.border, 1.5f);
			break;
		case BlockKind::Admonition:
			{
				const RGBA accent = admonitionColor(palette, mAdmonitionType);
				painter.fillRect(localRect(), Palette::mix(palette.surface, accent, 0.08f), radius);
				painter.fillRect(makeRect(0, 0, MarkdownStyle().quoteBarWidth, size().y), accent, 1.5f);
				break;
			}
		case BlockKind::TableCell: painter.fillRect(localRect(), palette.surface, 2); break;
		case BlockKind::Rule:
			painter.line(
				Vec2F(0.0f, size().y / 2), Vec2F(size().x, size().y / 2), palette.border, style.metrics.separatorThickness
			);
			break;
		default: break;
	}
}

ListMarker::ListMarker(Shape shape, halnf width, halnf lineHeight) :
	mShape(shape),
	mLineHeight(lineHeight) {
	setDebugName("list marker");
	setSizing(AxisSizing::fixed(width), AxisSizing::fixed(lineHeight));
}

void ListMarker::paint(Painter& painter) {
	const RGBA color = theme().palette.mutedText;
	const halnf radius = std::max(2.0f, mLineHeight * 0.13f);
	const Vec2F center(size().x - radius - 2, mLineHeight / 2);

	switch (mShape) {
		case Shape::Disc: painter.fillCircle(center, radius, color); break;
		case Shape::Circle: painter.strokeCircle(center, radius - 0.5f, color, 1.2f); break;
		case Shape::Square:
			painter.fillRect(
				makeRect(center.x - radius * 0.85f, center.y - radius * 0.85f, radius * 1.7f, radius * 1.7f), color, 1
			);
			break;
	}
}

MarkdownImage::MarkdownImage(std::string path, std::string alt, size_t sourceOffset) :
	mPath(std::move(path)),
	mAlt(std::move(alt)),
	mSourceOffset(sourceOffset) {
	setDebugName("markdown image");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	setTooltip(mAlt.empty() ? mPath : mAlt + " (" + mPath + ")");
}

void MarkdownImage::onAttached() {
	ImageLoader* loader = ui() ? ui()->imageLoader() : nullptr;
	mImage = loader ? loader->load(mPath) : LoadedImage{};
	requestLayout();
}

Vec2F MarkdownImage::displaySizeFor(halnf width) const {
	if (!mImage.isValid()) return { std::min(width, 260.0f), kMissingImageHeight };

	const halnf scale = width > 0 && width < mImage.size.x ? width / mImage.size.x : 1;
	return { std::round(mImage.size.x * scale), std::round(mImage.size.y * scale) };
}

Extent MarkdownImage::measureContent(Axis axis, halnf crossLength) const {
	if (axis == Axis::Horizontal) {
		const halnf natural = mImage.isValid() ? mImage.size.x : 260;
		return Extent::shrinkable(natural, std::min(natural, 48.0f));
	}
	return Extent::rigid(displaySizeFor(crossLength).y);
}

void MarkdownImage::paint(Painter& painter) {
	const Theme& style = theme();
	const Vec2F shown = displaySizeFor(size().x);
	const RectF box = makeRect(0, 0, shown.x, shown.y);

	if (mImage.isValid()) {
		painter.image(box, mImage.id, style.metrics.indicatorCornerRadius);
		return;
	}

	painter.fillRect(box, style.palette.raisedSurface, style.metrics.indicatorCornerRadius);
	painter.strokeRect(box, style.palette.border, style.metrics.indicatorCornerRadius);
	const std::string label = (mAlt.empty() ? std::string("image") : mAlt) + " \u2014 not found";
	painter.text(
		label, Insets::uniform(8).shrink(box), style.metrics.smallFontSize, TextAlign::Center, style.palette.mutedText
	);
}

void MarkdownImage::onEvent(Event& event) {
	if (event.type() != Event::Type::Click || !mOnActivated) return;

	mOnActivated(mSourceOffset);
	event.accept();
}

MarkdownView::MarkdownView() {
	setDebugName("markdown view");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<ColumnLayout>(mStyle.blockGap);
}

void MarkdownView::setMarkdown(std::string source) {
	if (source == mDocument.source() && !mTopBlocks.empty()) return;

	mDocument.setSource(std::move(source));
	rebuild();
}

void MarkdownView::setBaseDirectory(std::string directory) {
	if (directory == mBaseDirectory) return;

	mBaseDirectory = std::move(directory);
	if (!mTopBlocks.empty()) rebuild();
}

std::string MarkdownView::resolveImagePath(const std::string& source) const {
	const bool absolute = !source.empty() && source.front() == '/';
	const bool remote = source.find("://") != std::string::npos;
	if (absolute || remote || mBaseDirectory.empty()) return source;
	return mBaseDirectory + "/" + source;
}

std::string MarkdownView::resolveExternalLink(const std::string& link) const {
	const size_t colon = link.find(':');
	const bool hasScheme =
		colon != std::string::npos && colon > 1 &&
		std::all_of(link.begin(), link.begin() + (std::ptrdiff_t) colon, [](char character) {
			return std::isalnum((unsigned char) character) || character == '+' || character == '-' || character == '.';
		});
	if (hasScheme) return link;

	const std::string encoded = link.substr(0, link.find_first_of("?#"));
	if (encoded.empty()) return link;
	std::string path;
	for (size_t index = 0; index < encoded.size(); index++) {
		const bool escape = encoded[index] == '%' && index + 2 < encoded.size() &&
												std::isxdigit((unsigned char) encoded[index + 1]) &&
												std::isxdigit((unsigned char) encoded[index + 2]);
		if (!escape) {
			path += encoded[index];
			continue;
		}
		path += (char) std::stoi(encoded.substr(index + 1, 2), nullptr, 16);
		index += 2;
	}
	return std::filesystem::absolute(std::filesystem::path(resolveImagePath(path))).lexically_normal().string();
}

void MarkdownView::setFontScale(halnf scale) {
	scale = std::clamp(scale, kMinimumFontScale, kMaximumFontScale);
	if (scale == mFontScale) return;

	mFontScale = scale;
	if (!mTopBlocks.empty()) rebuild();
}

halnf MarkdownView::scaledFontSize(halnf scale) const { return theme().metrics.fontSize * scale * mFontScale; }

void MarkdownView::setStyle(const MarkdownStyle& style) {
	mStyle = style;
	if (auto* layout = dynamic_cast<ColumnLayout*>(this->layout())) layout->setGap(mStyle.blockGap);
	rebuild();
}

void MarkdownView::rebuild() {
	removeAllChildren();
	mRetired = std::move(mOwned);
	mOwned.clear();
	mTopBlocks.clear();

	for (const Block& block : mDocument.blocks()) {
		auto& widget = static_cast<MarkdownBlockWidget&>(build(block, *this, 0));
		mTopBlocks.push_back(&widget);
	}
	startTicking();
	requestLayout();
	requestPaint();
}

void MarkdownView::onTick(halnf) {
	stopTicking();
	mRetired.clear();

	if (!mPendingTaskToggle) return;
	const size_t offset = *mPendingTaskToggle;
	mPendingTaskToggle.reset();

	std::string toggled = mDocument.withTaskToggled(offset);
	setMarkdown(toggled);
	if (mOnTaskToggled) mOnTaskToggled(mDocument.source());
}

void MarkdownView::requestTaskToggle(size_t taskMarkOffset) {
	mPendingTaskToggle = taskMarkOffset;
	startTicking();
}

Widget& MarkdownView::build(const Block& block, Widget& parent, size_t depth) {
	const auto surface =
		block.kind == BlockKind::Table ? MarkdownBlockWidget::Surface::None : MarkdownBlockWidget::Surface::Automatic;
	auto& owner = own<MarkdownBlockWidget>(block, surface);
	parent.addChild(owner);
	const Theme& style = theme();

	switch (block.kind) {
		case BlockKind::Heading:
			{
				const size_t level = (size_t) std::clamp(block.headingLevel, 1, 6) - 1;
				owner.emplaceLayout<ColumnLayout>().setPadding({ 0, level < 2 ? 6.0f : 2.0f, 0, level < 2 ? 4.0f : 0.0f });
				addText(owner, block, mStyle.headingScale[level], FontFace::Bold);
				break;
			}
		case BlockKind::Paragraph:
			{
				owner.emplaceLayout<ColumnLayout>(mStyle.blockGap * 0.6f);
				addParagraph(owner, block);
				break;
			}
		case BlockKind::Code:
		case BlockKind::Html:
			{
				owner.emplaceLayout<ColumnLayout>().setPadding(Insets::uniform(mStyle.codePadding));
				addCode(owner, block);
				break;
			}
		case BlockKind::Quote:
			{
				owner.emplaceLayout<ColumnLayout>(mStyle.blockGap * 0.6f).setPadding({ mStyle.quoteIndent, 2, 0, 2 });
				buildChildren(block, owner, depth);
				for (RichText* text : textWidgets()) {
					if (owner.isSelfOrAncestorOf(*text)) text->setBaseColor(style.palette.mutedText);
				}
				break;
			}
		case BlockKind::Admonition:
			{
				owner.emplaceLayout<ColumnLayout>(6).setPadding({ mStyle.quoteIndent, 8, 10, 8 });
				auto& title = own<RichText>();
				title.setFontSize(scaledFontSize(1));
				title.setSpans(
					{ { titleCase(block.admonitionType), FontFace::Bold, admonitionColor(style.palette, block.admonitionType) } }
				);
				owner.addChild(title);
				buildChildren(block, owner, depth);
				break;
			}
		case BlockKind::BulletList:
		case BlockKind::OrderedList: buildList(block, owner, depth); break;
		case BlockKind::Rule:
			{
				owner.setSizing(AxisSizing::fill(), AxisSizing::fixed(9));
				break;
			}
		case BlockKind::Table:
			{
				owner.emplaceLayout<RowLayout>();
				auto& grid = own<MarkdownBlockWidget>(block, MarkdownBlockWidget::Surface::TableGrid);
				owner.addChild(grid);
				buildTable(block, grid);
				break;
			}
		default:
			{
				owner.emplaceLayout<ColumnLayout>(mStyle.blockGap);
				buildChildren(block, owner, depth);
				break;
			}
	}
	return owner;
}

void MarkdownView::buildChildren(const Block& block, Widget& owner, size_t depth) {
	for (const Block& child : block.children) {
		if (child.kind == BlockKind::Image) continue;
		build(child, owner, depth);
	}
}

void MarkdownView::buildList(const Block& list, Widget& owner, size_t depth) {
	owner.emplaceLayout<ColumnLayout>(list.tightList ? kListGap : mStyle.blockGap * 0.6f);

	unsigned number = list.listStart;
	for (const Block& item : list.children) {
		const std::optional<unsigned> itemNumber =
			list.kind == BlockKind::OrderedList ? std::optional<unsigned>(number++) : std::nullopt;
		buildListItem(item, itemNumber, owner, depth);
	}
}

void MarkdownView::buildListItem(
	const Block& item, const std::optional<unsigned>& number, Widget& owner, size_t depth
) {
	auto& row = own<MarkdownBlockWidget>(item);
	row.emplaceLayout<RowLayout>(kMarkerGap).setCrossAlign(CrossAlign::Start);
	owner.addChild(row);

	if (item.taskChecked) {
		auto& box = own<Checkbox>();
		box.setChecked(*item.taskChecked);
		box.setTooltip("Toggle task");
		const size_t offset = item.taskMarkOffset;
		box.setOnChanged([this, offset](bool) { requestTaskToggle(offset); });
		row.addChild(box);
	} else {
		const halnf markerWidth = mStyle.listIndent * mFontScale - kMarkerGap;
		if (number) {
			auto& numeral = own<RichText>();
			numeral.setFontSize(scaledFontSize(1));
			numeral.setSizing(AxisSizing::fixed(markerWidth), AxisSizing::fit());
			numeral.setAlignment(TextAlign::End);
			numeral.setBaseColor(theme().palette.mutedText);
			numeral.setSpans({ { std::to_string(*number) + "." } });
			row.addChild(numeral);
		} else {
			const ListMarker::Shape shapes[] = { ListMarker::Shape::Disc,
																					 ListMarker::Shape::Circle,
																					 ListMarker::Shape::Square };
			const halnf lineHeight = std::round(theme().metrics.lineHeight * mFontScale * 1.15f);
			row.addChild(own<ListMarker>(shapes[depth % 3], markerWidth, lineHeight));
		}
	}

	auto& body = own<Widget>();
	body.setSizing(AxisSizing::fill(), AxisSizing::fit());
	body.emplaceLayout<ColumnLayout>(kListGap);
	row.addChild(body);

	bool hasContent = false;
	if (!item.runs.empty()) {
		addParagraph(body, item);
		hasContent = true;
	}
	for (const Block& child : item.children) {
		if (child.kind == BlockKind::Image) continue;
		build(child, body, depth + (child.kind == BlockKind::BulletList || child.kind == BlockKind::OrderedList ? 1 : 0));
		hasContent = true;
	}
	if (!hasContent) addText(body, item, 1, FontFace::Regular);

	if (item.taskChecked && *item.taskChecked) {
		for (RichText* text : textWidgets()) {
			if (body.isSelfOrAncestorOf(*text)) text->setBaseColor(theme().palette.mutedText);
		}
	}
}

void MarkdownView::buildTable(const Block& table, Widget& owner) {
	const size_t columns = table.children.empty() ? 1 : table.children.front().children.size();
	owner.setLayout(std::make_unique<TableLayout>(columns));
	owner.layout()->setPadding(Insets::uniform(1));
	owner.setSizing(AxisSizing::fit(), AxisSizing::fit());

	for (const Block& row : table.children) {
		for (size_t column = 0; column < columns; column++) {
			if (column >= row.children.size()) {
				auto& filler = own<MarkdownBlockWidget>(row);
				owner.addChild(filler);
				continue;
			}

			const Block& cell = row.children[column];
			auto& cellWidget = own<MarkdownBlockWidget>(cell);
			cellWidget.emplaceLayout<ColumnLayout>().setPadding(
				Insets::symmetric(mStyle.cellPadding + 2, mStyle.cellPadding / 2 + 1)
			);
			owner.addChild(cellWidget);
			RichText& text = addText(cellWidget, cell, 1, row.headerRow ? FontFace::Bold : FontFace::Regular);
			text.setAlignment(textAlignFor(cell.align));
		}
	}
}

RichText& MarkdownView::addText(Widget& owner, const Block& block, halnf fontScale, FontFace baseFace) {
	auto& text = own<RichText>();
	text.setFontSize(scaledFontSize(fontScale));
	text.setLineSpacing(fontScale > 1.2f ? 1.05f : 1.15f);
	text.setSpans(spansOf(block, baseFace));
	text.setOnLinkActivated([this](const std::string& link) {
		if (mOnLink) mOnLink(link);
	});
	text.setOnSourceActivated([this](size_t offset) {
		if (mOnSource) mOnSource(offset);
	});
	owner.addChild(text);
	return text;
}

RichText& MarkdownView::addCode(Widget& owner, const Block& block) {
	auto& text = own<RichText>();
	text.setFontSize(scaledFontSize(1));
	std::vector<TextSpan> spans;
	size_t start = 0;
	while (start <= block.code.size()) {
		const size_t end = std::min(block.code.find('\n', start), block.code.size());
		if (!spans.empty()) spans.push_back({ .lineBreak = true });
		TextSpan line;
		line.text = block.code.substr(start, end - start);
		line.face = FontFace::Monospace;
		line.sourceOffset = block.source.begin;
		spans.push_back(line);
		start = end + 1;
	}
	text.setPreservesWhitespace(true);
	text.setSpans(std::move(spans));
	text.setOnSourceActivated([this](size_t offset) {
		if (mOnSource) mOnSource(offset);
	});
	owner.addChild(text);
	return text;
}

bool MarkdownView::isOnlyImages(const Block& block) const {
	bool hasImage = false;
	for (const Block& child : block.children)
		hasImage |= child.kind == BlockKind::Image;
	if (!hasImage) return false;

	for (const TextRun& run : block.runs) {
		const bool isImageLabel = std::any_of(block.children.begin(), block.children.end(), [&](const Block& child) {
			return child.kind == BlockKind::Image && run.link == child.imageSource && !run.link.empty();
		});
		const bool isBlank = run.hardBreak || run.text.find_first_not_of(" ") == std::string::npos;
		if (!isImageLabel && !isBlank) return false;
	}
	return true;
}

void MarkdownView::addParagraph(Widget& owner, const Block& block) {
	if (!isOnlyImages(block)) addText(owner, block, 1, FontFace::Regular);
	addImages(owner, block);
}

void MarkdownView::addImages(Widget& owner, const Block& block) {
	for (const Block& child : block.children) {
		if (child.kind != BlockKind::Image) continue;

		auto& image = own<MarkdownImage>(resolveImagePath(child.imageSource), child.imageAlt, child.source.begin);
		image.setOnActivated([this](size_t offset) {
			if (mOnSource) mOnSource(offset);
		});
		owner.addChild(image);
	}
}

std::vector<TextSpan> MarkdownView::spansOf(const Block& block, FontFace baseFace) const {
	const Palette& palette = theme().palette;
	std::vector<TextSpan> spans;
	for (const TextRun& run : block.runs) {
		TextSpan span;
		span.lineBreak = run.hardBreak;
		span.text = run.text;
		span.face = faceFor(run.style, baseFace);
		span.link = run.link;
		span.strikethrough = run.style.strikethrough;
		span.underline = run.style.underline;
		span.sourceOffset = run.source.isEmpty() ? block.source.begin : run.source.begin;
		if (run.style.code) span.background = codeChipColor(palette);
		if (run.style.highlight) span.background = highlightColor(palette);
		spans.push_back(std::move(span));
	}
	return spans;
}

std::vector<RichText*> MarkdownView::textWidgets() const {
	std::vector<RichText*> texts;
	for (const auto& widget : mOwned) {
		if (auto* text = dynamic_cast<RichText*>(widget.get())) texts.push_back(text);
	}
	return texts;
}

std::vector<MarkdownImage*> MarkdownView::images() const {
	std::vector<MarkdownImage*> found;
	for (const auto& widget : mOwned) {
		if (auto* image = dynamic_cast<MarkdownImage*>(widget.get())) found.push_back(image);
	}
	return found;
}

std::vector<Checkbox*> MarkdownView::taskBoxes() const {
	std::vector<Checkbox*> boxes;
	for (const auto& widget : mOwned) {
		if (auto* box = dynamic_cast<Checkbox*>(widget.get())) boxes.push_back(box);
	}
	return boxes;
}

std::optional<halnf> MarkdownView::offsetOfSource(size_t sourceOffset) const {
	const MarkdownBlockWidget* best = nullptr;
	for (const MarkdownBlockWidget* block : mTopBlocks) {
		if (block->source().begin > sourceOffset) break;
		best = block;
	}
	if (!best) return mTopBlocks.empty() ? std::nullopt : std::optional<halnf>(0);
	return best->rect().pos.y;
}

std::optional<size_t> MarkdownView::sourceAt(halnf localY) const {
	for (const MarkdownBlockWidget* block : mTopBlocks) {
		if (localY < block->rect().pos.y + block->rect().size.y) return block->source().begin;
	}
	if (mTopBlocks.empty()) return std::nullopt;
	return mTopBlocks.back()->source().begin;
}

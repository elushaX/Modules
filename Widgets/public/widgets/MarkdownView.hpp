#pragma once

#include "MarkdownDocument.hpp"
#include "RichText.hpp"
#include "Toggles.hpp"
#include "Widget.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	struct MarkdownStyle {
		halnf blockGap = 10;
		halnf listIndent = 22;
		halnf quoteIndent = 14;
		halnf quoteBarWidth = 3;
		halnf codePadding = 10;
		halnf cellPadding = 6;
		halnf headingScale[6] = { 1.85f, 1.5f, 1.25f, 1.1f, 1.0f, 0.9f };
	};

	class MarkdownBlockWidget : public Widget {
	public:
		enum class Surface { Automatic, None, TableGrid };

		explicit MarkdownBlockWidget(const markdown::Block& block, Surface surface = Surface::Automatic);

		[[nodiscard]] markdown::BlockKind kind() const { return mKind; }
		[[nodiscard]] const markdown::SourceRange& source() const { return mSource; }

	protected:
		void paint(Painter& painter) override;

	private:
		markdown::BlockKind mKind;
		Surface mSurface;
		markdown::SourceRange mSource;
		std::string mAdmonitionType;
	};

	class ListMarker : public Widget {
	public:
		enum class Shape { Disc, Circle, Square };

		ListMarker(Shape shape, halnf width, halnf lineHeight);

		[[nodiscard]] Shape shape() const { return mShape; }

	protected:
		void paint(Painter& painter) override;

	private:
		Shape mShape;
		halnf mLineHeight;
	};

	class MarkdownImage : public Widget {
	public:
		MarkdownImage(std::string path, std::string alt, size_t sourceOffset);

		[[nodiscard]] const std::string& path() const { return mPath; }
		[[nodiscard]] const std::string& alt() const { return mAlt; }
		[[nodiscard]] bool isLoaded() const { return mImage.isValid(); }
		[[nodiscard]] const LoadedImage& image() const { return mImage; }

		void setOnActivated(std::function<void(size_t sourceOffset)> callback) { mOnActivated = std::move(callback); }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		void onEvent(Event& event) override;
		void onAttached() override;

	private:
		[[nodiscard]] Vec2F displaySizeFor(halnf width) const;

		std::string mPath;
		std::string mAlt;
		size_t mSourceOffset;
		LoadedImage mImage;
		std::function<void(size_t)> mOnActivated;
	};

	class MarkdownView : public Widget {
	public:
		MarkdownView();

		void setMarkdown(std::string source);
		void setBaseDirectory(std::string directory);
		[[nodiscard]] const std::string& baseDirectory() const { return mBaseDirectory; }
		[[nodiscard]] std::string resolveImagePath(const std::string& source) const;
		[[nodiscard]] std::string resolveExternalLink(const std::string& link) const;
		[[nodiscard]] const std::string& markdown() const { return mDocument.source(); }
		[[nodiscard]] const markdown::MarkdownDocument& document() const { return mDocument; }

		void setStyle(const MarkdownStyle& style);
		void setFontScale(halnf scale);
		[[nodiscard]] halnf fontScale() const { return mFontScale; }
		[[nodiscard]] const MarkdownStyle& style() const { return mStyle; }

		[[nodiscard]] std::optional<halnf> offsetOfSource(size_t sourceOffset) const;
		[[nodiscard]] std::optional<size_t> sourceAt(halnf localY) const;
		[[nodiscard]] const std::vector<MarkdownBlockWidget*>& topBlocks() const { return mTopBlocks; }
		[[nodiscard]] std::vector<RichText*> textWidgets() const;
		[[nodiscard]] std::vector<Checkbox*> taskBoxes() const;
		[[nodiscard]] std::vector<MarkdownImage*> images() const;

		void setOnLinkActivated(std::function<void(const std::string&)> callback) { mOnLink = std::move(callback); }
		void setOnTaskToggled(std::function<void(const std::string& newSource)> callback) {
			mOnTaskToggled = std::move(callback);
		}
		void setOnSourceActivated(std::function<void(size_t)> callback) { mOnSource = std::move(callback); }

	protected:
		void onTick(halnf deltaMs) override;

	private:
		void rebuild();
		Widget& build(const markdown::Block& block, Widget& parent, size_t depth);
		void buildChildren(const markdown::Block& block, Widget& owner, size_t depth);
		void buildList(const markdown::Block& list, Widget& owner, size_t depth);
		void buildListItem(const markdown::Block& item, const std::optional<unsigned>& number, Widget& owner, size_t depth);
		void buildTable(const markdown::Block& table, Widget& owner);
		RichText& addText(Widget& owner, const markdown::Block& block, halnf fontScale, FontFace baseFace);
		RichText& addCode(Widget& owner, const markdown::Block& block);
		void addParagraph(Widget& owner, const markdown::Block& block);
		void addImages(Widget& owner, const markdown::Block& block);
		[[nodiscard]] bool isOnlyImages(const markdown::Block& block) const;
		[[nodiscard]] halnf scaledFontSize(halnf scale) const;
		[[nodiscard]] std::vector<TextSpan> spansOf(const markdown::Block& block, FontFace baseFace) const;
		void requestTaskToggle(size_t taskMarkOffset);

		template <typename Type, typename... Arguments>
		Type& own(Arguments&&... arguments) {
			auto widget = std::make_unique<Type>(std::forward<Arguments>(arguments)...);
			Type& reference = *widget;
			mOwned.push_back(std::move(widget));
			return reference;
		}

		markdown::MarkdownDocument mDocument;
		MarkdownStyle mStyle;
		std::string mBaseDirectory;
		halnf mFontScale = 1;
		std::vector<std::unique_ptr<Widget>> mOwned;
		std::vector<std::unique_ptr<Widget>> mRetired;
		std::vector<MarkdownBlockWidget*> mTopBlocks;
		std::optional<size_t> mPendingTaskToggle;

		std::function<void(const std::string&)> mOnLink;
		std::function<void(const std::string&)> mOnTaskToggled;
		std::function<void(size_t)> mOnSource;
	};
}

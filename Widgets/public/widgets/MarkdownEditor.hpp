#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "MarkdownView.hpp"
#include "TextArea.hpp"
#include "Theme.hpp"
#include "Views.hpp"
#include "WindowHost.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	[[nodiscard]] std::vector<HighlightSpan> highlightMarkdown(const std::string& source, const Palette& palette);
	[[nodiscard]] std::string
	imageParagraph(const std::string& source, size_t start, size_t end, const std::string& path);
	[[nodiscard]] std::optional<std::string> saveClipboardImage(const ClipboardImage& image, const std::string& directory);

	class MarkdownEditor : public Widget {
	public:
		enum class Mode { Editor, Split, Preview };

		MarkdownEditor();

		void setMarkdown(std::string source);
		[[nodiscard]] const std::string& markdown() const { return mSourceArea.text(); }
		void setBaseDirectory(std::string directory) { mPreview.setBaseDirectory(std::move(directory)); }

		static constexpr halnf kDefaultFontScale = 1.3f;

		void setFontScale(halnf scale);
		[[nodiscard]] halnf fontScale() const { return mFontScale; }
		void zoomBy(int steps);

		void setMode(Mode mode);
		[[nodiscard]] Mode mode() const { return mMode; }

		void setOnChanged(std::function<void(const std::string&)> callback) { mOnChanged = std::move(callback); }
		using LinkHandler = std::function<bool(const std::string& link)>;
		void setLinkHandler(LinkHandler handler) { mLinkHandler = std::move(handler); }
		bool openLink(const std::string& link);

		using ImageStore = std::function<std::optional<std::string>(const ClipboardImage& image)>;
		void setImageStore(ImageStore store) { mImageStore = std::move(store); }
		bool pasteImage(const ClipboardImage& image);

		[[nodiscard]] TextArea& sourceArea() { return mSourceArea; }
		[[nodiscard]] MarkdownView& preview() { return mPreview; }
		[[nodiscard]] ScrollView& previewScroll() { return mPreviewScroll; }
		[[nodiscard]] SplitView& split() { return mSplit; }
		[[nodiscard]] Button& modeButton(Mode mode);
		[[nodiscard]] Label& statusLabel() { return mStatus; }
		[[nodiscard]] bool showsSource() const { return mSourcePane.isVisible(); }
		[[nodiscard]] bool showsPreview() const { return mPreviewPane.isVisible(); }

		void revealSourceInEditor(size_t sourceOffset);
		void syncPreviewToSource();
		void syncSourceToPreview();

		struct ScrollAnchor {
			halnf source = 0;
			halnf preview = 0;
		};
		[[nodiscard]] std::vector<ScrollAnchor> scrollAnchors() const;

	protected:
		void onTick(halnf deltaMs) override;
		void filterDescendantEvent(Event& event) override;

	private:
		enum class Pane { Source, Preview };

		void sourceEdited(const std::string& source);
		void applyMode();
		void refreshStatus();
		void scrolled(Pane leader);
		void follow(Pane leader);
		bool revealHeading(const std::string& anchor);
		[[nodiscard]] bool opensLinkInSource(const Event& event) const;
		[[nodiscard]] size_t sourceAtTopOfEditor() const;
		[[nodiscard]] halnf previewContentTop() const;
		[[nodiscard]] std::optional<std::string> storeImage(const ClipboardImage& image) const;

		Mode mMode = Mode::Split;
		Panel mToolbar{ Panel::Surface::None };
		Button mEditorButton{ "Editor" };
		Button mSplitButton{ "Split" };
		Button mPreviewButton{ "Preview" };
		Spacer mToolbarSpacer;
		Label mStatus;

		SplitView mSplit{ Axis::Horizontal };
		Panel mSourcePane{ Panel::Surface::None };
		TextArea mSourceArea;
		Panel mPreviewPane{ Panel::Surface::Window };
		ScrollView mPreviewScroll;
		Widget mPreviewContent;
		MarkdownView mPreview;

		halnf mPendingRenderMs = -1;
		halnf mFontScale = 1;
		bool mFollowing = false;
		std::function<void(const std::string&)> mOnChanged;
		LinkHandler mLinkHandler;
		ImageStore mImageStore;
	};
}

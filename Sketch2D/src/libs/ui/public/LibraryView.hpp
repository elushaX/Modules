#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Dialog.hpp"
#include "Label.hpp"
#include "Menus.hpp"
#include "ReorderableList.hpp"
#include "SettingsView.hpp"
#include "SketchLibrary.hpp"
#include "TextField.hpp"

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace tp::sketch2d {

	class PreviewImages {
	public:
		virtual ~PreviewImages() = default;
		[[nodiscard]] virtual ui::ImageId image(const std::string& key, const SketchPreview& preview) = 0;
		[[nodiscard]] virtual ui::ImageId cached(const std::string& key) const = 0;
		virtual void forget(const std::string& key) = 0;
	};

	class SketchRow : public ui::Control {
	public:
		static constexpr halnf kPreviewSize = 72;

		SketchRow(LibraryItem item, ui::ImageId preview);

		[[nodiscard]] const LibraryItem& item() const { return mItem; }
		[[nodiscard]] bool isFolder() const { return mItem.kind == LibraryItem::Kind::Folder; }
		void setCurrent(bool current);
		void setShowsFolder(bool shows) { mShowsFolder = shows; }
		void setSelected(bool selected);
		[[nodiscard]] bool isSelected() const { return mSelected; }
		[[nodiscard]] bool isCurrent() const { return mCurrent; }

	protected:
		[[nodiscard]] ui::Extent measureContent(ui::Axis axis, halnf crossLength) const override;
		void paint(ui::Painter& painter) override;

	private:
		void paintFolderIcon(ui::Painter& painter, const RectF& box) const;

		LibraryItem mItem;
		ui::ImageId mPreview;
		bool mCurrent = false;
		bool mShowsFolder = false;
		bool mSelected = false;
	};

	class SaveSketchDialog : public ui::Dialog {
	public:
		explicit SaveSketchDialog(std::string title = "Save sketch", std::string confirm = "Save");

		void show(
			ui::Ui& ui,
			const std::string& suggestion,
			std::function<void(const std::string&)> onSave,
			ui::Widget* owner = nullptr
		);

		ui::TextField name;

	private:
		std::function<void(const std::string&)> mOnSave;
	};

	class MoreButton : public ui::PopupButton {
	public:
		MoreButton();

	protected:
		[[nodiscard]] bool hasIcon() const override { return true; }
		void paintIcon(ui::Painter& painter, const RectF& box) override;
	};

	class ModeBanner : public ui::Panel {
	public:
		ModeBanner();

		ui::Label message;
		ui::Button cancel{ "Cancel" };

	protected:
		void paint(ui::Painter& painter) override;
	};

	class LibraryList : public ui::Widget {
	public:
		LibraryList();

		std::unique_ptr<ModeBanner> banner;
		std::vector<std::unique_ptr<ui::Button>> bannerButtons;
		ui::Button* moveHere = nullptr;
		ui::Widget toolbar;
		ui::ScrollView pathScroll{ ui::Axis::Horizontal };
		ui::Widget path;
		std::vector<std::unique_ptr<ui::Button>> crumbs;
		ui::ReorderableList list;
		ui::Label empty;
		std::vector<std::unique_ptr<SketchRow>> rows;
		std::string folder;
		std::vector<std::string> selection;
		bool picksDestination = false;

		[[nodiscard]] SketchRow* row(const std::string& name) const;
	};

	class LibraryView : public ui::SettingsView {
	public:
		enum class Tab { Latest, Collections };

		LibraryView(SketchLibrary& library, Document& document);

		void setPreviewImages(PreviewImages* images) { mImages = images; }
		void open(ui::Ui& ui);
		void refresh();

		[[nodiscard]] Tab tab() const { return mTab; }
		void showTab(Tab tab);
		void openFolder(const std::string& folder);
		[[nodiscard]] LibraryList& currentList();
		[[nodiscard]] LibraryList& latestList() { return mLatest; }

		[[nodiscard]] const std::string& currentName() const { return mCurrentName; }
		void setCurrentName(std::string name) { mCurrentName = std::move(name); }
		void saveCurrent();
		void saveCurrentAs(const std::string& name);
		void openSketch(const std::string& path);
		void startNew(const std::string& folder = SketchLibrary::kScratchFolder);
		void createFolder(const std::string& parent, const std::string& name);
		void confirmRemove(const std::string& path);
		void remove(const std::string& path);
		void removeAll(const std::vector<std::string>& paths);
		void moveInto(const std::string& path, const std::string& folder);

		[[nodiscard]] bool isSelecting() const { return !mSelection.empty(); }
		[[nodiscard]] const std::vector<std::string>& selection() const { return mSelection; }
		void toggleSelected(const std::string& path);
		void clearSelection();
		void selectAll();
		void pickDestination();
		void moveSelectionTo(const std::string& folder);
		void confirmRemoveSelection();
		void confirmClearScratch();
		[[nodiscard]] std::vector<std::string> scratchSketches() const;
		void renameItem(const std::string& path, const std::string& name);
		void showRenameDialog(const std::string& path);
		[[nodiscard]] bool isPickingDestination() const { return mPickStartDepth > 0; }

		static constexpr halnf kAutosaveDelayMs = 5000;
		void setAutosave(bool enabled) { mAutosave = enabled; }
		[[nodiscard]] bool autosaves() const { return mAutosave; }
		void advanceAutosave(halnf deltaMs, bool busy);
		void saveIfChanged();
		void saveInBackground();
		void finishBackgroundSave(bool wait = true);
		[[nodiscard]] bool isSavingInBackground() const { return mSaving.valid(); }
		~LibraryView() override;
		void markSaved() { mSavedRevision = mDocument.history().revision(); }
		[[nodiscard]] bool hasUnsavedChanges() const { return mDocument.history().revision() != mSavedRevision; }

		ui::Button latestTab{ "Latest" };
		ui::Button collectionsTab{ "Collections" };
		MoreButton actionsButton;
		ui::Button clearScratch{ "Clear scratch" };
		ui::Menu actionsMenu;
		SaveSketchDialog saveDialog;
		SaveSketchDialog folderDialog{ "New folder", "Create" };
		SaveSketchDialog renameDialog{ "Rename", "Rename" };
		ui::Dialog confirmDialog{ "Delete" };

		std::function<void()> onDocumentReplaced;
		std::function<void(const std::string&)> onNotice;

	private:
		struct FolderPage {
			std::string folder;
			std::unique_ptr<LibraryList> list;
		};

		void notice(const std::string& text);
		void fill(LibraryList& list, const std::vector<LibraryItem>& items, bool reorderable);
		void refreshList(LibraryList& list);
		void wire(LibraryList& list);
		void placeActions();
		void rebuildActions();
		void showFolderDialog();
		void fillPath(LibraryList& list);
		void forgetPreview(const std::string& path);
		[[nodiscard]] std::string currentFolder() const;

		SketchLibrary& mLibrary;
		Document& mDocument;
		PreviewImages* mImages = nullptr;
		std::string mCurrentName;
		bool mAutosave = true;
		ualni mSavedRevision = 0;
		halnf mIdleMs = 0;
		std::future<bool> mSaving;
		std::string mSavingPath;
		ualni mSavingRevision = 0;
		Tab mTab = Tab::Latest;

		ui::Widget mRootPage;
		ui::Widget mTabs;
		ui::Widget mTabButtons;
		LibraryList mLatest;
		LibraryList mCollections;
		std::vector<FolderPage> mFolders;
		std::string mNewFolder = SketchLibrary::kScratchFolder;
		std::vector<std::string> mSelection;
		std::vector<std::string> mPendingRemovals;
		size_t mPickStartDepth = 0;
		void activateRow(const std::string& path, bool folder);
		void showSelection();
		void showSelectionBanner(LibraryList& list);
		ui::Button& addBannerButton(LibraryList& list, std::string text, std::function<void()> action);
		void openPicker(const std::string& folder);

	protected:
		void onEvent(ui::Event& event) override;
	};
}

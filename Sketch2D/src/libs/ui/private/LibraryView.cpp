#include "LibraryView.hpp"

#include "BoxLayout.hpp"
#include "Painter.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <ctime>

namespace tp::sketch2d {

	using namespace tp::ui;

	namespace {
		constexpr halnf kRowPadding = 10;

		std::string modifiedText(int8 seconds) {
			if (seconds <= 0) return {};
			const std::time_t time = (std::time_t) seconds;
			char text[64];
			std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", std::localtime(&time));
			return text;
		}

		std::string subtitleOf(const LibraryItem& item, bool showFolder) {
			if (item.kind == LibraryItem::Kind::Folder) return "Folder";
			const std::string modified = modifiedText(item.modifiedSeconds);
			const std::string folder = SketchLibrary::parentOf(item.path);
			return showFolder && !folder.empty() ? folder + "  ·  " + modified : modified;
		}
	}

	SketchRow::SketchRow(LibraryItem item, ImageId preview) :
		mItem(std::move(item)),
		mPreview(preview) {
		setDebugName("sketch row: " + mItem.path);
		setSizing(AxisSizing::fill(), AxisSizing::fit());
	}

	void SketchRow::setCurrent(bool current) {
		mCurrent = current;
		requestPaint();
	}

	void SketchRow::setSelected(bool selected) {
		mSelected = selected;
		requestPaint();
	}

	Extent SketchRow::measureContent(Axis axis, halnf) const {
		if (axis == Axis::Vertical) return Extent::rigid(kPreviewSize + 2 * kRowPadding);
		return Extent::natural(kPreviewSize + 2 * kRowPadding + textWidth(mItem.name, theme().metrics.fontSize));
	}

	void SketchRow::paintFolderIcon(Painter& painter, const RectF& box) const {
		const RGBA color = theme().palette.mutedText;
		const halnf unit = box.size.x / 8;
		const RectF body = makeRect(box.pos.x + unit, box.pos.y + unit * 2.5f, unit * 6, unit * 4.2f);
		const RectF tab = makeRect(box.pos.x + unit, box.pos.y + unit * 1.8f, unit * 2.4f, unit * 1.2f);
		painter.fillRect(tab, color, unit * 0.4f);
		painter.fillRect(body, color, unit * 0.5f);
	}

	void SketchRow::paint(Painter& painter) {
		const Theme& style = theme();
		painter.fillRect(localRect(), background(), style.metrics.cornerRadius);
		if (mCurrent || mSelected) painter.strokeRect(localRect(), style.palette.accent, style.metrics.cornerRadius, 2);
		if (mSelected) {
			const Vec2F mark = { size().x - kRowPadding - 14, size().y / 2 };
			painter.fillCircle(mark, 12, style.palette.accent);
			painter.polyline(
				{ mark + Vec2F(-5.0f, 0.0f), mark + Vec2F(-1.5f, 4.0f), mark + Vec2F(5.5f, -4.0f) },
				style.palette.onAccent,
				2.5f
			);
		}

		const RectF preview = makeRect(kRowPadding, kRowPadding, kPreviewSize, kPreviewSize);
		if (isFolder()) {
			paintFolderIcon(painter, preview);
		} else {
			painter.fillRect(preview, style.palette.field, style.metrics.indicatorCornerRadius);
			if (mPreview.isValid()) painter.image(preview, mPreview, style.metrics.indicatorCornerRadius);
		}

		const halnf textLeft = preview.pos.x + preview.size.x + kRowPadding * 1.5f;
		const halnf width = size().x - textLeft - kRowPadding;
		const halnf middle = size().y / 2;
		const halnf line = style.metrics.lineHeight;
		painter.text(
			mItem.name,
			makeRect(textLeft, middle - line, width, line),
			style.metrics.fontSize,
			TextAlign::Start,
			style.palette.text
		);
		painter.text(
			subtitleOf(mItem, mShowsFolder),
			makeRect(textLeft, middle, width, line),
			style.metrics.smallFontSize,
			TextAlign::Start,
			style.palette.mutedText
		);
	}

	SaveSketchDialog::SaveSketchDialog(std::string title, std::string confirm) :
		Dialog(std::move(title)) {
		setDebugName("name dialog: " + this->title());
		name.setMaximumLength(64);
		name.setOnCommitted([this](const std::string&) {
			if (isOpen()) accept();
		});
		body().addChild(name);
		addButton(
			std::move(confirm),
			[this]() {
				if (mOnSave) mOnSave(name.text());
			},
			true
		);
		addButton("Cancel", {});
	}

	void SaveSketchDialog::show(
		Ui& ui, const std::string& suggestion, std::function<void(const std::string&)> onSave, Widget* owner
	) {
		mOnSave = std::move(onSave);
		name.setText(suggestion);
		open(ui, owner);
		ui.setKeyboardFocus(&name);
		name.selectAll();
	}

	MoreButton::MoreButton() {
		setDebugName("more actions");
		setTooltip("More");
		setSizing(AxisSizing::fixed(48), AxisSizing::fixed(40));
	}

	void MoreButton::paintIcon(Painter& painter, const RectF&) {
		const Vec2F center = localRect().center();
		const RGBA color = theme().palette.text;
		for (halnf offset : { -8.0f, 0.0f, 8.0f })
			painter.fillCircle(center + Vec2F(offset, 0.0f), 2.5f, color);
	}

	ModeBanner::ModeBanner() :
		Panel(Surface::None) {
		setDebugName("mode banner");
		setSizing(AxisSizing::fill(), AxisSizing::fit());
		RowLayout& row = emplaceLayout<RowLayout>(10);
		row.setCrossAlign(CrossAlign::Center);
		row.setPadding(Insets::symmetric(14, 10));
		message.setSizing(AxisSizing::fill(), AxisSizing::fit());
		addChild(message);
		addChild(cancel);
	}

	void ModeBanner::paint(Painter& painter) {
		const Theme& style = theme();
		const RGBA accent = style.palette.accent;
		painter.fillRect(localRect(), { accent.r, accent.g, accent.b, 0.18f }, style.metrics.cornerRadius);
		painter.strokeRect(localRect(), accent, style.metrics.cornerRadius, 2);
	}

	LibraryList::LibraryList() :
		list(6) {
		setDebugName("library list");
		setSizing(AxisSizing::fill(), AxisSizing::fit());
		emplaceLayout<ColumnLayout>(10);
		list.setDragMode(ReorderableList::DragMode::LongPress);
		empty.setTone(Label::Tone::Muted);
		toolbar.setSizing(AxisSizing::fill(), AxisSizing::fit());
		toolbar.emplaceLayout<RowLayout>(8).setCrossAlign(CrossAlign::Center);
		path.setSizing(AxisSizing::fit(), AxisSizing::fit());
		path.emplaceLayout<RowLayout>(6).setCrossAlign(CrossAlign::Center);
		pathScroll.setSizing(AxisSizing::fill(), AxisSizing::fixed(40));
		pathScroll.setContent(&path);
		toolbar.addChild(pathScroll);
		addChild(toolbar);
		addChild(empty);
		addChild(list);
	}

	SketchRow* LibraryList::row(const std::string& name) const {
		for (const auto& candidate : rows)
			if (candidate->item().name == name) return candidate.get();
		return nullptr;
	}

	LibraryView::LibraryView(SketchLibrary& library, Document& document) :
		mLibrary(library),
		mDocument(document) {
		setDebugName("sketch library");

		mTabs.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mTabs.emplaceLayout<RowLayout>(8).setCrossAlign(CrossAlign::Center);
		mTabButtons.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mTabButtons.emplaceLayout<RowLayout>(6);
		for (Button* tabButton : { &latestTab, &collectionsTab }) {
			tabButton->setStyle(Button::Style::Tab);
			tabButton->setSizing(AxisSizing::fill(), AxisSizing::fit());
			mTabButtons.addChild(*tabButton);
		}
		mTabs.addChild(mTabButtons);
		clearScratch.setStyle(Button::Style::Flat);
		clearScratch.setTooltip("Delete every sketch in the Scratch folder");
		clearScratch.setOnActivate([this]() { confirmClearScratch(); });
		mTabs.addChild(clearScratch);
		actionsButton.setPopupContent(&actionsMenu);
		actionsButton.popupOptions().placement = Placement::Below;
		actionsButton.popupOptions().align = PlacementAlign::End;
		actionsButton.setOnOpening([this]() { rebuildActions(); });
		actionsMenu.setOnItemActivated([this]() { actionsButton.closePopup(); });
		latestTab.setOnActivate([this]() { showTab(Tab::Latest); });
		collectionsTab.setOnActivate([this]() { showTab(Tab::Collections); });

		mRootPage.setDebugName("library root");
		mRootPage.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mRootPage.emplaceLayout<ColumnLayout>(12);
		mRootPage.addChild(mTabs);
		mRootPage.addChild(mLatest);
		mRootPage.addChild(mCollections);

		mLatest.empty.setText("No sketches yet. Draw something - it is saved to Scratch automatically.");
		mCollections.empty.setText("No folders yet.");
		mLatest.toolbar.setVisible(false);
		wire(mCollections);
		wire(mLatest);

		confirmDialog.addButton(
			"Delete",
			[this]() {
				const std::vector<std::string> paths = std::move(mPendingRemovals);
				mPendingRemovals.clear();
				mSelection.clear();
				removeAll(paths);
			},
			true
		);
		confirmDialog.addCancelButton();

		setRootContent("Sketches", mRootPage);
		setOnPageShown([this](SettingsPage& page) {
			const bool leftPicker = isPickingDestination() && depth() <= mPickStartDepth;
			if (leftPicker) mPickStartDepth = 0;
			if (!isPickingDestination() && !leftPicker) mSelection.clear();
			while (!mFolders.empty() && mFolders.back().list.get() != &currentList() && depth() <= mFolders.size())
				mFolders.pop_back();
			refreshList(currentList());
			placeActions();
		});
	}

	void LibraryView::wire(LibraryList& list) {
		list.list.setOnHeld([this, &list](size_t index) {
			if (!list.picksDestination) toggleSelected(list.rows[index]->item().path);
		});
		list.list.setCanAcceptDrop([&list](size_t, size_t target) { return list.rows[target]->isFolder(); });
		list.list.setOnDroppedOnto([this, &list](size_t dragged, size_t target) {
			moveInto(list.rows[dragged]->item().path, list.rows[target]->item().path);
		});
		list.list.setOnMoved([this, &list](size_t from, size_t to) {
			std::unique_ptr<SketchRow> moved = std::move(list.rows[from]);
			list.rows.erase(list.rows.begin() + (std::ptrdiff_t) from);
			list.rows.insert(list.rows.begin() + (std::ptrdiff_t) to, std::move(moved));
			std::vector<std::string> names;
			for (const auto& row : list.rows)
				names.push_back(row->item().name);
			mLibrary.reorder(list.folder, names);
		});
	}

	void LibraryView::open(Ui& ui) {
		mLibrary.ensureScratch();
		mFolders.clear();
		mSelection.clear();
		mPickStartDepth = 0;
		showTab(mTab);
		SettingsView::open(ui);
		placeActions();
	}

	void LibraryView::showTab(Tab tab) {
		if (tab != mTab) mSelection.clear();
		mTab = tab;
		latestTab.setSelected(tab == Tab::Latest);
		collectionsTab.setSelected(tab == Tab::Collections);
		mLatest.setVisible(tab == Tab::Latest);
		mCollections.setVisible(tab == Tab::Collections);
		refreshList(tab == Tab::Latest ? mLatest : mCollections);
		if (isOpen() && depth() == 1) placeActions();
	}

	LibraryList& LibraryView::currentList() {
		if (depth() > 1 && depth() - 1 <= mFolders.size()) return *mFolders[depth() - 2].list;
		return mTab == Tab::Latest ? mLatest : mCollections;
	}

	std::string LibraryView::currentFolder() const {
		if (depth() > 1 && depth() - 1 <= mFolders.size()) return mFolders[depth() - 2].folder;
		return SketchLibrary::kScratchFolder;
	}

	void LibraryView::openFolder(const std::string& folder) {
		mSelection.clear();
		FolderPage page{ folder, std::make_unique<LibraryList>() };
		page.list->folder = folder;
		page.list->empty.setText("This folder is empty.");
		wire(*page.list);
		LibraryList& list = *page.list;
		mFolders.push_back(std::move(page));
		refreshList(list);
		openContent(SketchLibrary::nameOf(folder), list);
		placeActions();
	}

	void LibraryView::placeActions() {
		actionsButton.closePopup();
		if (actionsButton.parent()) actionsButton.parent()->removeChild(actionsButton);
		if (depth() > 1) currentList().toolbar.addChild(actionsButton);
		else mTabs.addChild(actionsButton);
		fillPath(currentList());
	}

	void LibraryView::fillPath(LibraryList& list) {
		list.path.removeAllChildren();
		list.crumbs.clear();
		const auto addCrumb = [this, &list](const std::string& text, size_t targetDepth, bool current) {
			auto crumb = std::make_unique<Button>(text);
			const halnf lift = 0.04f * (halnf) list.crumbs.size();
			const RGBA base = theme().palette.raisedSurface;
			crumb->setStyle(Button::Style::Flat);
			crumb->setSelected(current);
			crumb->setFill(RGBA{ base.r + lift, base.g + lift, base.b + lift, 1.0f });
			crumb->setOnActivate([this, targetDepth]() {
				if (targetDepth < depth()) backTo(targetDepth);
			});
			list.path.addChild(*crumb);
			list.crumbs.push_back(std::move(crumb));
		};

		const size_t first = isPickingDestination() ? mPickStartDepth : 0;
		const size_t rootDepth = isPickingDestination() ? mPickStartDepth + 1 : 1;
		addCrumb(isPickingDestination() ? "Move to" : "Collections", rootDepth, depth() == rootDepth);
		for (size_t index = first; index + 1 < depth() && index < mFolders.size(); index++) {
			if (mFolders[index].folder.empty()) continue;
			addCrumb(SketchLibrary::nameOf(mFolders[index].folder), index + 2, index + 2 == depth());
		}
		list.pathScroll.scrollTo(list.pathScroll.maximumOffset() + 10000);
	}

	void LibraryView::rebuildActions() {
		actionsMenu.clear();
		if (isPickingDestination()) {
			const std::string target = currentList().folder;
			actionsMenu.addItem("Move here", [this, target]() { moveSelectionTo(target); });
			actionsMenu.addItem("Cancel", [this]() { backTo(mPickStartDepth); });
			return;
		}
		if (isSelecting()) {
			MenuItem& count = actionsMenu.addItem(std::to_string(mSelection.size()) + " selected", {});
			count.setEnabled(false);
			actionsMenu.addItem("Move to…", [this]() { pickDestination(); });
			if (mSelection.size() == 1) {
				const std::string path = mSelection.front();
				actionsMenu.addItem("Rename…", [this, path]() { showRenameDialog(path); });
			}
			actionsMenu.addItem("Delete", [this]() { confirmRemoveSelection(); });
			actionsMenu.addSeparator();
			actionsMenu.addItem("Select all", [this]() { selectAll(); });
			actionsMenu.addItem("Cancel selection", [this]() { clearSelection(); });
			return;
		}
		const bool inFolder = depth() > 1;
		const std::string folder = currentFolder();
		actionsMenu.addItem("New sketch", [this, folder]() { startNew(folder); });
		if (inFolder || mTab == Tab::Collections) actionsMenu.addItem("New folder", [this]() { showFolderDialog(); });
		if (!inFolder) actionsMenu.addItem("Save current", [this]() { saveCurrent(); });
		if (folder == SketchLibrary::kScratchFolder && !scratchSketches().empty())
			actionsMenu.addItem("Clear scratch", [this]() { confirmClearScratch(); });
		if (inFolder && folder != SketchLibrary::kScratchFolder) {
			actionsMenu.addSeparator();
			actionsMenu.addItem("Delete folder", [this, folder]() { confirmRemove(folder); });
		}
	}

	void LibraryView::showFolderDialog() {
		if (!ui()) return;
		const std::string parent = depth() > 1 ? currentFolder() : std::string();
		folderDialog.show(
			*ui(), "New folder", [this, parent](const std::string& name) { createFolder(parent, name); }, &actionsButton
		);
	}

	void LibraryView::refresh() {
		if (!isOpen()) return;
		refreshList(currentList());
		if (depth() > 1) refreshList(mTab == Tab::Latest ? mLatest : mCollections);
	}

	void LibraryView::refreshList(LibraryList& list) {
		if (&list == &mLatest) {
			std::vector<LibraryItem> items;
			for (const SketchEntry& entry : mLibrary.list())
				items.push_back({ LibraryItem::Kind::Sketch, entry.name, entry.path, entry.directory, entry.modifiedSeconds });
			fill(list, items, false);
			return;
		}
		fill(list, mLibrary.children(list.folder), true);
	}

	void LibraryView::fill(LibraryList& list, const std::vector<LibraryItem>& items, bool reorderable) {
		list.list.clearItems();
		list.rows.clear();
		for (const LibraryItem& item : items) {
			ImageId image;
			if (mImages && item.kind == LibraryItem::Kind::Sketch) {
				image = mImages->cached(item.directory);
				if (!image.isValid()) {
					if (std::optional<SketchPreview> preview = SketchFile::loadPreview(item.directory))
						image = mImages->image(item.directory, *preview);
				}
			}
			if (list.picksDestination && item.kind != LibraryItem::Kind::Folder) continue;
			const bool moving = std::find(mSelection.begin(), mSelection.end(), item.path) != mSelection.end();
			if (list.picksDestination && moving) continue;
			auto row = std::make_unique<SketchRow>(item, image);
			row->setShowsFolder(&list == &mLatest);
			const std::string path = item.path;
			const bool folder = item.kind == LibraryItem::Kind::Folder;
			if (list.picksDestination) row->setOnActivate([this, path]() { openPicker(path); });
			else row->setOnActivate([this, path, folder]() { activateRow(path, folder); });
			row->setCurrent(path == mCurrentName && !list.picksDestination);
			row->setSelected(moving && !list.picksDestination);
			list.list.addItem(*row);
			list.rows.push_back(std::move(row));
		}
		const bool draggable = reorderable && !list.picksDestination;
		list.list.setDragMode(draggable ? ReorderableList::DragMode::LongPress : ReorderableList::DragMode::None);
		list.empty.setVisible(list.rows.empty());
		showSelectionBanner(list);
	}

	void LibraryView::forgetPreview(const std::string& path) {
		if (mImages) mImages->forget(mLibrary.directoryOf(path));
	}

	void LibraryView::saveCurrent() {
		if (!mCurrentName.empty()) {
			saveCurrentAs(mCurrentName);
			return;
		}
		if (!ui()) return;
		Widget* owner = isOpen() ? &actionsButton : nullptr;
		const std::string suggestion = SketchLibrary::nameOf(mLibrary.unusedName());
		saveDialog.show(*ui(), suggestion, [this](const std::string& name) { saveCurrentAs(name); }, owner);
	}

	void LibraryView::saveCurrentAs(const std::string& name) {
		finishBackgroundSave(true);
		const bool hasFolder = name.find('/') != std::string::npos;
		const std::string path = SketchLibrary::sanitizedPath(hasFolder ? name : SketchLibrary::joined(mNewFolder, name));
		if (!mLibrary.save(path, mDocument)) {
			notice("Could not save " + SketchLibrary::nameOf(path));
			return;
		}
		forgetPreview(path);
		mCurrentName = path;
		markSaved();
		notice("Saved " + SketchLibrary::nameOf(path));
		refresh();
	}

	void LibraryView::advanceAutosave(halnf deltaMs, bool busy) {
		finishBackgroundSave(false);
		busy = busy || mDocument.paintBacklog() > 0;
		mIdleMs = busy || !hasUnsavedChanges() ? 0 : mIdleMs + deltaMs;
		if (mIdleMs >= kAutosaveDelayMs) saveInBackground();
	}

	void LibraryView::saveIfChanged() {
		saveInBackground();
		finishBackgroundSave(true);
	}

	void LibraryView::saveInBackground() {
		mIdleMs = 0;
		if (!mAutosave || !hasUnsavedChanges()) return;
		if (mCurrentName.empty() && mDocument.tileCount() == 0) return;
		if (mSaving.valid()) return;
		const std::string path = mCurrentName.empty() ? mLibrary.unusedPath(mNewFolder, "Sketch") : mCurrentName;
		auto file = std::make_shared<const SketchFile>(SketchFile::capture(mDocument));
		const SketchLibrary* library = &mLibrary;
		mSavingPath = path;
		mSavingRevision = mDocument.history().revision();
		mSaving = std::async(std::launch::async, [library, path, file]() { return library->save(path, *file); });
	}

	void LibraryView::finishBackgroundSave(bool wait) {
		if (!mSaving.valid()) return;
		if (!wait && mSaving.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
		if (!mSaving.get()) return;
		forgetPreview(mSavingPath);
		if (mCurrentName.empty() || mCurrentName == mSavingPath) {
			mCurrentName = mSavingPath;
			mSavedRevision = mSavingRevision;
		}
		refresh();
	}

	LibraryView::~LibraryView() {
		if (mSaving.valid()) mSaving.wait();
	}

	void LibraryView::openSketch(const std::string& path) {
		if (path != mCurrentName) saveIfChanged();
		if (!mLibrary.open(path, mDocument)) {
			notice("Could not open " + SketchLibrary::nameOf(path));
			return;
		}
		mCurrentName = path;
		markSaved();
		if (onDocumentReplaced) onDocumentReplaced();
		close();
	}

	void LibraryView::startNew(const std::string& folder) {
		saveIfChanged();
		mDocument.reset();
		mCurrentName.clear();
		mNewFolder = folder;
		markSaved();
		if (onDocumentReplaced) onDocumentReplaced();
		close();
	}

	void LibraryView::createFolder(const std::string& parent, const std::string& name) {
		const bool blank = name.find_first_not_of(' ') == std::string::npos;
		const std::string path = blank ? mLibrary.unusedPath(parent, "New folder")
																	 : SketchLibrary::joined(parent, SketchLibrary::sanitizedName(name));
		if (!mLibrary.createFolder(path)) {
			notice("Could not create " + SketchLibrary::nameOf(path));
			return;
		}
		refresh();
	}

	void LibraryView::confirmRemove(const std::string& path) {
		if (!ui()) return;
		mPendingRemovals = { path };
		const bool folder = mLibrary.isFolder(path);
		confirmDialog.setTitle(folder ? "Delete folder?" : "Delete sketch?");
		confirmDialog.setMessage(
			folder ? "\"" + SketchLibrary::nameOf(path) + "\" and everything inside it will be deleted."
						 : "\"" + SketchLibrary::nameOf(path) + "\" will be deleted."
		);
		confirmDialog.open(*ui(), &actionsButton);
	}

	void LibraryView::remove(const std::string& path) { removeAll({ path }); }

	void LibraryView::removeAll(const std::vector<std::string>& paths) {
		finishBackgroundSave(true);
		bool viewingRemoved = false;
		for (const std::string& path : paths) {
			const bool viewingIt = depth() > 1 && (currentFolder() == path || currentFolder().starts_with(path + "/"));
			if (!mLibrary.remove(path)) {
				notice("Could not delete " + SketchLibrary::nameOf(path));
				continue;
			}
			viewingRemoved = viewingRemoved || viewingIt;
			if (mCurrentName == path || mCurrentName.starts_with(path + "/")) mCurrentName.clear();
			forgetPreview(path);
		}
		if (viewingRemoved) back();
		else refresh();
	}

	void LibraryView::moveInto(const std::string& path, const std::string& folder) {
		finishBackgroundSave(true);
		if (!mLibrary.move(path, folder)) {
			notice(
				"Could not move " + SketchLibrary::nameOf(path) + " - \"" + SketchLibrary::nameOf(folder) +
				"\" already has an item with that name or is inside it"
			);
			return;
		}
		const std::string moved = SketchLibrary::joined(folder, SketchLibrary::nameOf(path));
		if (mCurrentName == path) mCurrentName = moved;
		else if (mCurrentName.starts_with(path + "/")) mCurrentName = moved + mCurrentName.substr(path.size());
		forgetPreview(path);
		refresh();
	}

	void LibraryView::activateRow(const std::string& path, bool folder) {
		if (isSelecting()) {
			toggleSelected(path);
			return;
		}
		if (folder) openFolder(path);
		else openSketch(path);
	}

	void LibraryView::toggleSelected(const std::string& path) {
		if (path == SketchLibrary::kScratchFolder) {
			notice("Scratch always stays - open it to move or delete what is inside");
			return;
		}
		const auto found = std::find(mSelection.begin(), mSelection.end(), path);
		if (found == mSelection.end()) mSelection.push_back(path);
		else mSelection.erase(found);
		showSelection();
	}

	void LibraryView::clearSelection() {
		mSelection.clear();
		showSelection();
	}

	void LibraryView::selectAll() {
		for (const auto& row : currentList().rows)
			if (
				row->item().path != SketchLibrary::kScratchFolder &&
				std::find(mSelection.begin(), mSelection.end(), row->item().path) == mSelection.end()
			)
				mSelection.push_back(row->item().path);
		showSelection();
	}

	void LibraryView::showSelection() {
		LibraryList& list = currentList();
		for (const auto& row : list.rows) {
			const bool selected = std::find(mSelection.begin(), mSelection.end(), row->item().path) != mSelection.end();
			row->setSelected(selected);
		}
		showSelectionBanner(list);
	}

	Button& LibraryView::addBannerButton(LibraryList& list, std::string text, std::function<void()> action) {
		list.bannerButtons.push_back(std::make_unique<Button>(std::move(text)));
		Button& button = *list.bannerButtons.back();
		button.setOnActivate(std::move(action));
		list.banner->addChild(button);
		return button;
	}

	void LibraryView::showSelectionBanner(LibraryList& list) {
		if (list.picksDestination) return;
		if (!isSelecting()) {
			if (list.banner) list.removeChild(*list.banner);
			list.bannerButtons.clear();
			list.banner.reset();
			return;
		}
		if (!list.banner) {
			list.banner = std::make_unique<ModeBanner>();
			list.banner->cancel.setOnActivate([this]() { clearSelection(); });
			addBannerButton(list, "Delete", [this]() { confirmRemoveSelection(); });
			addBannerButton(list, "Move to\u2026", [this]() { pickDestination(); }).setStyle(Button::Style::Accent);
			list.insertChild(0, *list.banner);
		}
		const size_t count = mSelection.size();
		list.banner->message.setText(
			(count == 1 ? std::string("1 item selected") : std::to_string(count) + " items selected") +
			" - tap rows to add or remove"
		);
	}

	void LibraryView::pickDestination() {
		if (!isSelecting()) return;
		mPickStartDepth = depth();
		openPicker("");
	}

	void LibraryView::openPicker(const std::string& folder) {
		FolderPage page{ folder, std::make_unique<LibraryList>() };
		page.list->folder = folder;
		page.list->picksDestination = true;
		page.list->empty.setText("No folders inside. Tap Move here to put the items in this folder.");
		page.list->banner = std::make_unique<ModeBanner>();
		ModeBanner& banner = *page.list->banner;
		const size_t count = mSelection.size();
		const std::string items =
			count == 1 ? "\"" + SketchLibrary::nameOf(mSelection.front()) + "\"" : std::to_string(count) + " items";
		const bool anySketch = std::any_of(mSelection.begin(), mSelection.end(), [this](const std::string& path) {
			return mLibrary.contains(path);
		});
		const bool allowed = !(folder.empty() && anySketch);
		const std::string place = folder.empty() ? "the top level" : "\"" + SketchLibrary::nameOf(folder) + "\"";
		banner.message.setText(
			allowed ? "Moving " + items + " - pick a folder, then tap Move here (into " + place + ")"
							: "Moving " + items + " - sketches must go into a folder, pick one below"
		);
		page.list->moveHere = &addBannerButton(*page.list, "Move here", [this, folder]() { moveSelectionTo(folder); });
		page.list->moveHere->setEnabled(allowed);
		page.list->moveHere->setStyle(Button::Style::Accent);
		banner.cancel.setOnActivate([this]() { backTo(mPickStartDepth); });
		page.list->insertChild(1, banner);
		LibraryList& list = *page.list;
		mFolders.push_back(std::move(page));
		refreshList(list);
		openContent(folder.empty() ? "Move to" : SketchLibrary::nameOf(folder), list);
		placeActions();
	}

	void LibraryView::moveSelectionTo(const std::string& folder) {
		const size_t returnDepth = mPickStartDepth;
		mPickStartDepth = 0;
		size_t failed = 0;
		for (const std::string& path : mSelection) {
			if (SketchLibrary::parentOf(path) == folder) continue;
			const bool isSketch = mLibrary.contains(path);
			if (folder.empty() && isSketch) {
				failed++;
				continue;
			}
			finishBackgroundSave(true);
			if (!mLibrary.move(path, folder)) {
				failed++;
				continue;
			}
			const std::string moved = SketchLibrary::joined(folder, SketchLibrary::nameOf(path));
			if (mCurrentName == path) mCurrentName = moved;
			else if (mCurrentName.starts_with(path + "/")) mCurrentName = moved + mCurrentName.substr(path.size());
			forgetPreview(path);
		}
		mSelection.clear();
		if (failed > 0) {
			notice(
				"Could not move " + std::to_string(failed) + (failed == 1 ? " item" : " items") +
				" - the name is already used there or a folder cannot go inside itself"
			);
		}
		backTo(returnDepth);
		refresh();
	}

	void LibraryView::confirmRemoveSelection() {
		if (!ui() || !isSelecting()) return;
		mPendingRemovals = mSelection;
		const size_t count = mPendingRemovals.size();
		confirmDialog.setTitle(count == 1 ? "Delete 1 item?" : "Delete " + std::to_string(count) + " items?");
		confirmDialog.setMessage("Selected folders are deleted with everything inside them.");
		confirmDialog.open(*ui(), &actionsButton);
	}

	std::vector<std::string> LibraryView::scratchSketches() const {
		std::vector<std::string> paths;
		for (const LibraryItem& item : mLibrary.children(SketchLibrary::kScratchFolder))
			if (item.kind == LibraryItem::Kind::Sketch) paths.push_back(item.path);
		return paths;
	}

	void LibraryView::confirmClearScratch() {
		if (!ui()) return;
		mPendingRemovals = scratchSketches();
		if (mPendingRemovals.empty()) {
			notice("Scratch is already empty");
			return;
		}
		const size_t count = mPendingRemovals.size();
		confirmDialog.setTitle("Clear scratch?");
		confirmDialog.setMessage(
			(count == 1 ? std::string("1 sketch") : std::to_string(count) + " sketches") +
			" in Scratch will be deleted. Sketches in other folders stay."
		);
		confirmDialog.open(*ui(), &clearScratch);
	}

	void LibraryView::showRenameDialog(const std::string& path) {
		if (!ui()) return;
		renameDialog.show(
			*ui(),
			SketchLibrary::nameOf(path),
			[this, path](const std::string& name) { renameItem(path, name); },
			&actionsButton
		);
	}

	void LibraryView::renameItem(const std::string& path, const std::string& name) {
		const std::string clean = SketchLibrary::sanitizedName(name);
		finishBackgroundSave(true);
		if (!mLibrary.rename(path, clean)) {
			notice("Could not rename to " + clean + " - that name is taken here");
			return;
		}
		const std::string renamed = SketchLibrary::joined(SketchLibrary::parentOf(path), clean);
		if (mCurrentName == path) mCurrentName = renamed;
		else if (mCurrentName.starts_with(path + "/")) mCurrentName = renamed + mCurrentName.substr(path.size());
		forgetPreview(path);
		mSelection.clear();
		refresh();
		if (depth() > 1) refreshList(mTab == Tab::Latest ? mLatest : mCollections);
	}

	void LibraryView::onEvent(Event& event) {
		const bool escape = event.type() == Event::Type::KeyDown && event.keyCode() == InputID::ESCAPE;
		if (escape && isSelecting() && !isPickingDestination()) {
			clearSelection();
			event.accept();
			return;
		}
		SettingsView::onEvent(event);
	}

	void LibraryView::notice(const std::string& text) {
		if (onNotice) onNotice(text);
	}
}

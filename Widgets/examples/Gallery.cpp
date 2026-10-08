#include "Gallery.hpp"

#include "Theme.hpp"
#include "Ui.hpp"

#include "BoxLayout.hpp"
#include "StackLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace tp;
using namespace tp::ui;
using namespace tp::ui::gallery;

namespace {
	constexpr halnf kPageSpacing = 12;

	std::string percent(halnf fraction) { return std::to_string((int) std::round(fraction * 100)) + "%"; }

	std::string colorText(const RGBA& color) {
		char buffer[48];
		std::snprintf(buffer, sizeof(buffer), "rgb %.2f %.2f %.2f", color.r, color.g, color.b);
		return buffer;
	}
}

ControlsPage::ControlsPage(GalleryState& state) :
	mState(state),
	mSecondSeparator(Axis::Horizontal) {
	setDebugName("controls page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));
	layout.setCrossAlign(CrossAlign::Start);

	mButtonRow.emplaceLayout<RowLayout>(8).setCrossAlign(CrossAlign::Center);
	clickMe.setOnActivate([this]() {
		mState.buttonClicks++;
		refreshClickCount();
	});
	clickMe.setTooltip("Counts how many times it was clicked");
	accent.setStyle(Button::Style::Accent);
	disabled.setEnabled(false);
	mButtonRow.addChild(clickMe);
	mButtonRow.addChild(accent);
	mButtonRow.addChild(disabled);
	mButtonRow.addChild(clickCount);
	refreshClickCount();

	notifications.setChecked(mState.notificationsEnabled);
	notifications.setOnChanged([this](bool checked) { mState.notificationsEnabled = checked; });
	darkMode.setChecked(mState.darkMode);
	darkMode.setOnChanged([this](bool checked) { mState.darkMode = checked; });

	mRadioRow.emplaceLayout<RowLayout>(16);
	for (RadioButton* option : { &low, &medium, &high }) {
		quality.add(*option);
		mRadioRow.addChild(*option);
	}
	quality.selectIndex((size_t) mState.qualityIndex);
	quality.setOnChanged([this](size_t index) { mState.qualityIndex = (long) index; });

	volume.setSizing(AxisSizing::fixed(260), AxisSizing::fit());
	volume.setValueFormatter(percent);
	volume.setValue(mState.volume);
	volume.setOnChanged([this](halnf value) {
		mState.volume = value;
		progress.setFraction(value);
	});

	brushSize.setSizing(AxisSizing::fixed(260), AxisSizing::fit());
	brushSize.setScale(Slider::Scale::Logarithmic);
	brushSize.setValueFormatter([](halnf value) { return std::to_string((int) std::lround(value)) + " px"; });

	progress.setSizing(AxisSizing::fixed(260), AxisSizing::fit());
	progress.setFraction(mState.volume);
	blendMode.setTooltip("Layer blend mode");

	accentColor.setTooltip("Accent color");
	accentColor.setOnSelected([this](long index) {
		const RGBA choices[] = { accents::blue, accents::purple, accents::green, accents::orange, accents::pink };
		if (index >= 0 && index < 5 && ui()) ui()->setTheme(Theme::standard().withAccent(choices[index]));
	});

	addChild(mButtonRow);
	addChild(mFirstSeparator);
	addChild(notifications);
	addChild(darkMode);
	addChild(mRadioRow);
	addChild(mSecondSeparator);
	addChild(volume);
	addChild(brushSize);
	addChild(progress);
	addChild(blendMode);
	addChild(accentColor);
}

void ControlsPage::refreshClickCount() {
	clickCount.setText("clicked " + std::to_string(mState.buttonClicks) + " times");
}

InputPage::InputPage(GalleryState& state) :
	mState(state) {
	setDebugName("input page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));
	layout.setCrossAlign(CrossAlign::Start);

	name.setText(mState.userName);
	name.setPlaceholder("Type a name");
	name.setSizing(AxisSizing::fixed(260), AxisSizing::fit());
	name.setOnEdited([this](const std::string& text) {
		mState.userName = text;
		refresh();
	});

	size.setSuffix(" px");
	size.setValue(mState.brushSize);
	size.setSizing(AxisSizing::fixed(260), AxisSizing::fit());
	size.setOnChanged([this](halnf value) {
		mState.brushSize = value;
		refresh();
	});

	dialog.setMessage("Widgets gallery: text, number fields and dialogs.");
	dialog.addButton("Close", {}, true);
	openDialog.setOnActivate([this]() {
		if (ui()) dialog.open(*ui());
	});
	askConfirm.setOnActivate([this]() {
		if (!ui()) return;
		confirm.ask(*ui(), "Clear history", "Are you sure you want to clear history?", "Clear history", [this]() {
			mState.confirmations++;
			refresh();
		});
	});

	showNotice.setOnActivate([this]() { notice.show(*this, "Saved " + mState.userName + "'s settings"); });

	notes.setPlaceholder("Multiline text: Enter adds a line, Ctrl+Enter commits");
	notes.setSizing(Axis::Horizontal, AxisSizing::fixed(360));
	notes.setBoxHeight(110);
	color.setPalette({ accents::blue, accents::purple, accents::green, accents::orange, accents::pink });
	color.setColor(accents::blue);

	mDialogRow.emplaceLayout<RowLayout>(8).setCrossAlign(CrossAlign::Center);
	mDialogRow.addChild(openDialog);
	mDialogRow.addChild(askConfirm);
	mDialogRow.addChild(showNotice);
	mDialogRow.addChild(confirmations);

	addChild(nameLabel);
	addChild(name);
	addChild(sizeLabel);
	addChild(size);
	addChild(greeting);
	addChild(notesLabel);
	addChild(notes);
	addChild(colorLabel);
	addChild(color);
	addChild(mDialogRow);
	refresh();
}

void InputPage::refresh() {
	char size[32];
	std::snprintf(size, sizeof(size), "%.0f", (double) mState.brushSize);
	greeting.setText(
		"Hello, " + (mState.userName.empty() ? std::string("stranger") : mState.userName) + " - brush " + size + " px"
	);
	confirmations.setText("confirmed " + std::to_string(mState.confirmations) + " times");
}

FilesPage::FilesPage(FileSystem& fileSystem) :
	browser(fileSystem),
	dialog(fileSystem) {
	setDebugName("files page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));

	hint.setTone(Label::Tone::Muted);
	status.setTone(Label::Tone::Muted);
	browser.setSizing(AxisSizing::fill(), AxisSizing::fill());
	browser.setDirectory(fileSystem.home());
	browser.onSelected = [this](const FileEntry& entry) {
		status.setText("Selected " + joinPath(browser.directory(), entry.name));
	};
	browser.onFileOpened = [this](const std::string& path) { report("Opened", path); };

	actions.setSizing(AxisSizing::fill(), AxisSizing::fit());
	actions.emplaceLayout<RowLayout>(8);
	openFile.setOnActivate([this]() {
		if (!ui()) return;
		dialog.show(*ui(), FileDialog::Mode::Open, "Open file", browser.directory(), {}, [this](const std::string& path) {
			report("Opened", path);
		});
	});
	saveFile.setOnActivate([this]() {
		if (!ui()) return;
		dialog.show(
			*ui(), FileDialog::Mode::Save, "Save as", browser.directory(), "untitled.txt", [this](const std::string& path) {
				report("Would save to", path);
			}
		);
	});
	chooseFolder.setOnActivate([this]() {
		if (!ui()) return;
		dialog.show(
			*ui(),
			FileDialog::Mode::ChooseDirectory,
			"Choose folder",
			browser.directory(),
			{},
			[this](const std::string& path) {
				report("Chose", path);
				browser.setDirectory(path);
			}
		);
	});
	actions.addChild(openFile);
	actions.addChild(saveFile);
	actions.addChild(chooseFolder);

	addChild(hint);
	addChild(browser);
	addChild(actions);
	addChild(status);
}

void FilesPage::report(const std::string& action, const std::string& path) { status.setText(action + " " + path); }

const char* const MarkdownPage::kSampleDirectory = "rsc";

const char* const MarkdownPage::kSampleDocument = R"md(# Markdown editor

Write on the **left**, read on the *right*. Scrolling either side keeps the other on the
same section, and clicking rendered text puts the caret on its source line. Use the buttons above to switch between
**Editor**, **Split** and **Preview**, like in an IDE.

## Inline styles

Plain text with **bold**, *italic*, ***both***, `inline code`, ~~strikethrough~~,
_underline_ and ==highlight==. Links look like [the md4c parser](https://github.com/mity/md4c)
and autolinks like https://example.com.\
A backslash at the end of a line forces a hard break.

Clicking a link in the preview, or Ctrl+clicking it in the source, opens it in the default desktop app:
web pages in the browser, [local files](markdown/pipeline.png) in their viewer, and
[headings in this document](#images) scroll into view.

## Lists

1. Ordered lists keep their numbers
2. Even when nested:
   - bullets change shape
     - at every level
3. And they wrap long lines neatly under the text instead of under the number marker.

### Tasks

- [x] Parse markdown with md4c
- [x] Render it with our own widgets
- [ ] Click a checkbox: the source changes too

## Quotes and callouts

> Simplicity is prerequisite for reliability.
> — Edsger W. Dijkstra

> [!TIP]
> GitHub style admonitions: NOTE, TIP, IMPORTANT, WARNING and CAUTION.

> [!WARNING]
> Saving overwrites the chosen file.

## Code

```cpp
#include "MarkdownEditor.hpp"

tp::ui::MarkdownEditor editor;
editor.setMarkdown("# Hello");
editor.setMode(tp::ui::MarkdownEditor::Mode::Split);
```

## Tables

| Feature        | Status | Notes                       |
|:---------------|:------:|----------------------------:|
| Headings       |   yes  | six levels                  |
| Tables         |   yes  | left, center, right columns |
| Images         |   yes  | scaled to the column width  |

## Images

Image paths are resolved against the open file's folder, or `rsc` for this sample.
Large images shrink to the column width; missing ones show their alt text.

![Rendering pipeline](markdown/pipeline.png)

![A missing picture](markdown/missing.png)

---

Hold **Ctrl** and scroll to zoom both panes together.
)md";

MarkdownPage::MarkdownPage(FileSystem& fileSystem) :
	dialog(fileSystem),
	mFileSystem(fileSystem),
	mDirectory(fileSystem.home()) {
	setDebugName("markdown page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(8);
	layout.setPadding(Insets::uniform(12));

	actions.setSizing(AxisSizing::fill(), AxisSizing::fit());
	actions.emplaceLayout<RowLayout>(8).setCrossAlign(CrossAlign::Center);
	status.setTone(Label::Tone::Muted);
	status.setText("Sample document");

	openFile.setOnActivate([this]() {
		if (!ui()) return;
		dialog.show(*ui(), FileDialog::Mode::Open, "Open markdown", mDirectory, {}, [this](const std::string& path) {
			open(path);
		});
	});
	saveFile.setOnActivate([this]() {
		if (!ui()) return;
		dialog.show(
			*ui(), FileDialog::Mode::Save, "Save markdown", mDirectory, "notes.md", [this](const std::string& path) {
				save(path);
			}
		);
	});
	reset.setOnActivate([this]() { showSample(); });
	editor.setImageStore([this](const ClipboardImage& image) { return storePastedImage(image); });
	showSample();

	actions.addChild(openFile);
	actions.addChild(saveFile);
	actions.addChild(reset);
	actions.addChild(status);
	addChild(actions);
	addChild(editor);
}

void MarkdownPage::showSample() {
	mOpenedFile = false;
	editor.setBaseDirectory(kSampleDirectory);
	editor.setMarkdown(kSampleDocument);
	status.setText("Sample document");
}

std::optional<std::string> MarkdownPage::storePastedImage(const ClipboardImage& image) {
	const std::string folder = mOpenedFile ? mDirectory : std::string(kSampleDirectory);
	const std::string relativeFolder = "images";
	const std::optional<std::string> saved = saveClipboardImage(image, joinPath(folder, relativeFolder));
	if (!saved) return std::nullopt;
	return relativeFolder + "/" + std::filesystem::path(*saved).filename().string();
}

void MarkdownPage::open(const std::string& path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		status.setText("Cannot open " + path);
		return;
	}

	std::ostringstream content;
	content << file.rdbuf();
	mDirectory = parentPath(path);
	mOpenedFile = true;
	editor.setBaseDirectory(mDirectory);
	editor.setMarkdown(content.str());
	status.setText("Opened " + path);
}

void MarkdownPage::save(const std::string& path) {
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file || !(file << editor.markdown())) {
		status.setText("Cannot save " + path);
		return;
	}

	const std::string previousFolder = mOpenedFile ? mDirectory : std::string(kSampleDirectory);
	mDirectory = parentPath(path);
	mOpenedFile = true;
	copyPastedImages(previousFolder, mDirectory);
	editor.setBaseDirectory(mDirectory);
	status.setText("Saved " + path);
}

void MarkdownPage::copyPastedImages(const std::string& fromFolder, const std::string& toFolder) {
	if (fromFolder == toFolder) return;

	const std::string& source = editor.markdown();
	for (size_t at = source.find("](images/pasted-"); at != std::string::npos;
			 at = source.find("](images/pasted-", at + 2)) {
		const size_t end = source.find(')', at);
		if (end == std::string::npos) break;

		const std::string relative = source.substr(at + 2, end - at - 2);
		const std::string target = joinPath(toFolder, relative);
		if (mFileSystem.exists(target)) continue;
		if (!mFileSystem.isDirectory(joinPath(toFolder, "images")))
			mFileSystem.createDirectory(joinPath(toFolder, "images"));

		std::ifstream in(joinPath(fromFolder, relative), std::ios::binary);
		std::ofstream out(target, std::ios::binary | std::ios::trunc);
		if (in && out) out << in.rdbuf();
	}
}

MediaPage::MediaPage() :
	playlist(
		{ { "Jumpsuit", "twenty one pilots", "Trench", 238.5f, true },
			{ "Mystery Lady", "Masego & Don Toliver", "Studying Abroad", 205, false },
			{ "No Idea", "Don Toliver", "Heaven or Hell", 154, true } }
	) {
	setDebugName("media page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));

	hint.setTone(Label::Tone::Muted);
	status.setTone(Label::Tone::Muted);
	player.onPlayPause = [this]() { togglePlaying(); };
	player.onPrevious = [this]() { load((current + playlist.size() - 1) % playlist.size()); };
	player.onNext = [this]() { load((current + 1) % playlist.size()); };
	player.seek.setOnSeek([this](halnf position) {
		status.setText("Seeked to " + formatDuration(position * playlist[current].durationSeconds));
	});
	player.volume.setOnChanged([this](halnf volume) {
		status.setText("Volume " + std::to_string((int) std::lround(volume * 100)) + "%");
	});

	addChild(hint);
	addChild(player);
	addChild(status);
	load(0);
}

void MediaPage::load(size_t index) {
	current = index;
	player.setTrack(&playlist[current]);
	player.setPosition(0);
	player.setBuffered(1);
	status.setText("Loaded \"" + playlist[current].title + "\"");
}

void MediaPage::togglePlaying() {
	player.setPlaying(!player.isPlaying());
	if (player.isPlaying()) startTicking();
	else stopTicking();
	status.setText(player.isPlaying() ? "Playing" : "Paused");
}

void MediaPage::onTick(halnf deltaMs) {
	const halnf duration = playlist[current].durationSeconds;
	if (!player.isPlaying() || duration <= 0 || player.seek.isScrubbing()) return;

	const halnf position = player.seek.position() + deltaMs / 1000.0f / duration;
	if (position < 1) {
		player.setPosition(position);
		return;
	}
	load((current + 1) % playlist.size());
	player.setPlaying(true);
}

ReorderPage::ReorderPage() {
	setDebugName("reorder page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));
	layout.setCrossAlign(CrossAlign::Start);

	hint.setTone(Label::Tone::Muted);
	list.setSizing(AxisSizing::fixed(320), AxisSizing::fit());
	for (const char* task : { "Sketch", "Line art", "Flat colors", "Shading", "Highlights", "Background" }) {
		tasks.push_back(std::make_unique<Checkbox>(task));
		tasks.back()->setSizing(AxisSizing::fill(), AxisSizing::fit());
		list.addItem(*tasks.back());
	}
	list.setOnMoved([this](size_t from, size_t to) {
		status.setText(
			"Moved \"" + static_cast<Checkbox&>(list.item(to)).text() + "\" from " + std::to_string(from + 1) + " to " +
			std::to_string(to + 1)
		);
	});
	status.setTone(Label::Tone::Muted);

	addChild(hint);
	addChild(list);
	addChild(status);
	refreshStatus();
}

void ReorderPage::refreshStatus() { status.setText("Nothing moved yet"); }

ListPage::ListPage() {
	setDebugName("list page");
	emplaceLayout<StackLayout>();

	content.emplaceLayout<ColumnLayout>(0).setPadding(Insets::uniform(8));
	content.setSizing(AxisSizing::fill(), AxisSizing::fit());

	const char* const groupNames[] = { "Brushes", "Layers", "Documents" };
	const char* const subgroupNames[] = { "Recent", "Favorites", "Archived" };
	for (const char* groupName : groupNames) {
		groups.push_back(std::make_unique<Collapsible>(groupName));
		Collapsible& group = *groups.back();
		group.header().setSummary(std::to_string(std::size(subgroupNames)) + " folders");
		for (const char* subgroupName : subgroupNames) {
			subgroups.push_back(std::make_unique<Collapsible>(subgroupName));
			Collapsible& subgroup = *subgroups.back();
			constexpr int kItems = 5;
			const bool reorderable = std::string(subgroupName) == "Favorites";
			subgroup.header().setSummary(std::to_string(kItems) + (reorderable ? " · drag to sort" : ""));
			ReorderableList* sortable = nullptr;
			if (reorderable) {
				favoriteLists.push_back(std::make_unique<ReorderableList>());
				sortable = favoriteLists.back().get();
				subgroup.body().addChild(*sortable);
			}
			for (int item = 0; item < kItems; item++) {
				groupRows.push_back(
					std::make_unique<Button>(std::string(groupName) + " / " + subgroupName + " " + std::to_string(item + 1))
				);
				groupRows.back()->setStyle(Button::Style::Flat);
				groupRows.back()->setSizing(AxisSizing::fill(), AxisSizing::fit());
				if (sortable) sortable->addItem(*groupRows.back());
				else subgroup.body().addChild(*groupRows.back());
			}
			group.body().addChild(subgroup);
		}
		list.addItem(group);
	}

	for (int index = 0; index < 60; index++) {
		rows.push_back(std::make_unique<Button>("Row " + std::to_string(index)));
		rows.back()->setStyle(Button::Style::Flat);
		rows.back()->setSizing(AxisSizing::fill(), AxisSizing::fit());
		allRows.body().addChild(*rows.back());
	}
	allRows.header().setSummary(std::to_string(rows.size()));
	list.addItem(allRows);

	const char* const steps[] = { "Sketch", "Line art", "Flat colors", "Shading", "Highlights" };
	for (const char* step : steps) {
		draggableItems.push_back(std::make_unique<Checkbox>(step));
		draggableItems.back()->setSizing(AxisSizing::fill(), AxisSizing::fit());
		draggable.addItem(*draggableItems.back());

		staticItems.push_back(std::make_unique<Checkbox>(step));
		staticItems.back()->setSizing(AxisSizing::fill(), AxisSizing::fit());
		staticList.addChild(*staticItems.back());
	}
	draggableStatus.setTone(Label::Tone::Muted);
	draggableStatus.setText("Drag the grip or the row to reorder");
	draggable.setOnMoved([this](size_t from, size_t to) {
		draggableStatus.setText(
			"Moved \"" + static_cast<Checkbox&>(draggable.item(to)).text() + "\" from " + std::to_string(from + 1) + " to " +
			std::to_string(to + 1)
		);
	});
	draggableSection.header().setSummary("reorderable");
	draggableSection.body().addChild(draggable);
	draggableSection.body().addChild(draggableStatus);
	list.addItem(draggableSection);

	staticList.emplaceLayout<ColumnLayout>(2);
	staticList.setSizing(AxisSizing::fill(), AxisSizing::fit());
	staticSection.header().setSummary("fixed order");
	staticSection.body().addChild(staticList);
	list.addItem(staticSection);
	content.addChild(list);

	scroll.setContent(&content);
	addChild(scroll);
}

CanvasPage::CanvasPage(GalleryState& state) :
	mState(state) {
	setDebugName("canvas page");
	emplaceLayout<StackLayout>();

	desk.emplaceLayout<FreeLayout>();
	wheel.setColor(mState.brushColor);
	wheel.setOnChanged([this](const RGBA& color) {
		mState.brushColor = color;
		refreshReadout();
	});
	colorPanel.setContent(&wheel);
	colorPanel.setPanelSize({ 240, 260 });
	colorPanel.requestPosition({ 24, 24 });
	desk.addChild(colorPanel);

	inspector.emplaceLayout<ColumnLayout>(8).setPadding(Insets::uniform(12));
	inspector.addChild(inspectorTitle);
	inspector.addChild(colorReadout);
	refreshReadout();

	split.setFirst(&desk);
	split.setSecond(&inspector);
	split.setRatio(0.7f);
	addChild(split);
}

void CanvasPage::refreshReadout() { colorReadout.setText(colorText(mState.brushColor)); }

DockPage::DockPage() {
	setDebugName("dock page");
	emplaceLayout<StackLayout>().setPadding(Insets::uniform(Theme::standard().metrics.surfaceGap));

	editor.emplaceLayout<StackLayout>().setPadding(Insets::uniform(16));
	editor.addChild(editorText);
	editor.setSizing(AxisSizing::fill(), AxisSizing::fill());
	dock.setCenter(&editor);

	project.setContent(&projectText);
	structure.setContent(&structureText);
	terminal.setContent(&terminalText);
	properties.setContent(&propertiesText);
	terminal.setDockedLength(160);

	for (DockPanel* panel : { &project, &structure, &terminal, &properties })
		dock.addPanel(*panel);
	dock.openPanel(project);
	dock.openPanel(terminal);

	addChild(dock);
}

NodesPage::NodesPage() {
	setDebugName("nodes page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));

	hint.setTone(Label::Tone::Muted);
	hint.setWrap(true);
	hint.setSizing(AxisSizing::fill(), AxisSizing::fit());
	status.setTone(Label::Tone::Muted);

	graph.setOnChanged([this] { refreshStatus(); });
	editor.setOnSelectionChanged([this] { refreshStatus(); });
	populate();

	addChild(hint);
	addChild(editor);
	addChild(status);
	refreshStatus();
}

void NodesPage::populate() {
	const RGBA colorPort = { 0.85f, 0.75f, 0.30f, 1.0f };
	const RGBA valuePort = { 0.55f, 0.75f, 0.95f, 1.0f };

	const NodeTypeId texture = graph.registerType(
		{ "Texture",
			{ 0.30f, 0.45f, 0.30f, 1.0f },
			180,
			{ { "uv", "value", valuePort } },
			{ { "color", "color", colorPort } } }
	);
	const NodeTypeId mix = graph.registerType(
		{ "Mix",
			{ 0.35f, 0.30f, 0.50f, 1.0f },
			180,
			{ { "a", "color", colorPort, false },
				{ "b", "color", colorPort, false },
				{ "factor", "value", valuePort, false } },
			{ { "result", "color", colorPort } } }
	);
	const NodeTypeId output = graph.registerType(
		{ "Output", { 0.55f, 0.30f, 0.25f, 1.0f }, 160, { { "surface", "color", colorPort, false } }, {} }
	);

	const NodeId wood = graph.addNode(texture, { 40, 40 });
	const NodeId rust = graph.addNode(texture, { 40, 220 });
	const NodeId blend = graph.addNode(mix, { 300, 110 });
	const NodeId surface = graph.addNode(output, { 560, 140 });

	graph.connect({ wood, PortDirection::Output, 0 }, { blend, PortDirection::Input, 0 });
	graph.connect({ rust, PortDirection::Output, 0 }, { blend, PortDirection::Input, 1 });
	graph.connect({ blend, PortDirection::Output, 0 }, { surface, PortDirection::Input, 0 });
}

namespace {
	enum KanbanDemoColumn : size_t { Ideas, InProgress, Review, Released };

	const RGBA kKanbanFeature = { 0.21f, 0.42f, 0.79f, 1.0f };
	const RGBA kKanbanBug = { 0.70f, 0.26f, 0.24f, 1.0f };
	const RGBA kKanbanChore = { 0.25f, 0.55f, 0.32f, 1.0f };
}

CarouselPage::CarouselPage() {
	setDebugName("carousel page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));

	hint.setTone(Label::Tone::Muted);
	hint.setWrap(true);
	hint.setSizing(AxisSizing::fill(), AxisSizing::fit());

	colorsText.setText("Colors: the first page. Press the right arrow to see the notes.");
	colorsText.setWrap(true);
	colorsText.setSizing(AxisSizing::fill(), AxisSizing::fit());
	notesText.setText("Notes: the last page wraps around to the first one.");
	notesText.setWrap(true);
	notesText.setSizing(AxisSizing::fill(), AxisSizing::fit());

	longList.setSizing(AxisSizing::fill(), AxisSizing::fit());
	longList.emplaceLayout<ColumnLayout>(4);
	for (int row = 1; row <= 60; row++) {
		longListRows.push_back(std::make_unique<Label>("Row " + std::to_string(row) + " scrolls vertically"));
		longList.addChild(*longListRows.back());
	}
	longListScroll.setSizing(AxisSizing::fill(), AxisSizing::fill());
	longListScroll.setContent(&longList);

	carousel.setSizing(AxisSizing::fill(), AxisSizing::fill());
	carousel.addTab("Colors", colorsText);
	carousel.addTab("List", longListScroll);
	carousel.addTab("Notes", notesText);

	addChild(hint);
	addChild(carousel);
}

KanbanPage::KanbanPage() {
	setDebugName("kanban page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));

	hint.setTone(Label::Tone::Muted);
	hint.setWrap(true);
	hint.setSizing(AxisSizing::fill(), AxisSizing::fit());
	status.setTone(Label::Tone::Muted);
	board.setSizing(AxisSizing::fill(), AxisSizing::fill());

	populate();
	board.setCardUpdater([this](KanbanCard& card) {
		const Story& story = model.stories.at(card.id());
		card.setTitle(story.title);
		card.setDetail(story.detail);
		card.setAccent(story.accent);
	});
	board.setOnMoved([this](const KanbanMove& move) {
		model.apply(move);
		refreshStatus("Moved \"" + model.stories.at(move.item).title + "\" to " + board.columns()[move.toColumn].title);
	});
	board.setOnSelected([this](KanbanItemId id) { refreshStatus("Selected \"" + model.stories.at(id).title + "\""); });
	board.setOnActivated([this](KanbanItemId id) { refreshStatus("Opened \"" + model.stories.at(id).title + "\""); });
	board.setColumns(
		{
			{ "Ideas", RGBA{ 0.45f, 0.47f, 0.52f, 1.0f } },
			{ "In progress", RGBA{ 0.75f, 0.60f, 0.18f, 1.0f } },
			{ "Review", RGBA{ 0.55f, 0.36f, 0.81f, 1.0f }, 2 },
			{ "Released", RGBA{ 0.25f, 0.55f, 0.32f, 1.0f }, std::nullopt, true, KanbanOrdering::ModelDefined },
		}
	);

	addChild(hint);
	addChild(board);
	addChild(status);
	refreshStatus("Nothing moved yet");
}

void KanbanPage::populate() {
	const std::vector<std::pair<size_t, Story>> stories = {
		{ Ideas, { "Dark theme for the editor", "Follow the system appearance", kKanbanFeature } },
		{ Ideas, { "Export to PDF", "", kKanbanFeature } },
		{ Ideas, { "Tidy build scripts", "Drop the unused presets", kKanbanChore } },
		{ InProgress, { "Crash when closing a docked panel", "Only with two windows open", kKanbanBug } },
		{ InProgress, { "Keyboard shortcuts page", "", kKanbanFeature } },
		{ Review, { "Faster text layout", "Cache shaped runs per paragraph", kKanbanFeature } },
		{ Released, { "Undo for node moves", "", kKanbanFeature } },
		{ Released, { "Scroll bar flicker", "", kKanbanBug } },
	};

	model.columns.assign(4, {});
	KanbanItemId next = 1;
	for (const auto& [column, story] : stories) {
		model.stories[next] = story;
		model.columns[column].push_back(next++);
	}
}

bool KanbanPage::Model::canMove(const KanbanMove& move) const {
	if (move.toColumn != Released || move.fromColumn == Released) return true;
	return move.fromColumn == Review;
}

void KanbanPage::Model::apply(const KanbanMove& move) {
	auto& from = columns[move.fromColumn];
	from.erase(std::find(from.begin(), from.end(), move.item));
	auto& to = columns[move.toColumn];
	if (move.toColumn == Released) {
		to.push_back(move.item);
		std::sort(to.begin(), to.end());
		return;
	}
	to.insert(to.begin() + std::ptrdiff_t(std::min(move.toIndex, to.size())), move.item);
}

void KanbanPage::refreshStatus(const std::string& action) {
	status.setText(action + "  -  " + std::to_string(model.columns[Released].size()) + " released");
}

TreePage::TreePage() {
	setDebugName("tree page");
	ColumnLayout& layout = emplaceLayout<ColumnLayout>(kPageSpacing);
	layout.setPadding(Insets::uniform(16));

	hint.setTone(Label::Tone::Muted);
	hint.setWrap(true);
	hint.setSizing(AxisSizing::fill(), AxisSizing::fit());
	status.setTone(Label::Tone::Muted);
	view.setSizing(AxisSizing::fill(), AxisSizing::fill());

	populate();
	view.setCardFactory([this](TreeItemId id) { return createCard(id); });
	view.setCardUpdater([this](TreeCard& card) { updateCard(card); });
	view.setOnSelected([this](TreeItemId) { refreshStatus(); });
	view.setOnCreateChild([this](TreeItemId parent) {
		const TreeItemId child = model.parents.rbegin()->first + 1;
		model.parents[child] = parent;
		view.modelChanged();
		view.select(child);
		refreshStatus();
	});
	view.setOnReparent([this](TreeItemId child, TreeItemId parent) {
		model.parents[child] = parent;
		view.modelChanged();
		refreshStatus();
	});
	view.modelChanged();

	addChild(hint);
	addChild(view);
	addChild(status);
	refreshStatus();
}

void TreePage::populate() {
	TreeItemId next = 1;
	model.parents[next] = std::nullopt;
	std::vector<TreeItemId> level = { next++ };
	for (size_t depth = 1; depth < kLevels; depth++) {
		std::vector<TreeItemId> deeper;
		for (size_t index = 0; index < level.size(); index++) {
			const size_t count = depth < 3 ? 3 : (index % 3 == 0 ? 2 : 1);
			for (size_t child = 0; child < count && deeper.size() < 40; child++) {
				model.parents[next] = level[index];
				deeper.push_back(next++);
			}
		}
		level = deeper;
	}
}

std::vector<TreeItemId> TreePage::Model::roots() const {
	std::vector<TreeItemId> result;
	for (const auto& [id, parent] : parents)
		if (!parent) result.push_back(id);
	return result;
}

std::vector<TreeItemId> TreePage::Model::children(TreeItemId id) const {
	std::vector<TreeItemId> result;
	for (const auto& [child, parent] : parents)
		if (parent == id) result.push_back(child);
	return result;
}

size_t TreePage::Model::depthOf(TreeItemId id) const {
	size_t depth = 0;
	for (auto parent = parents.at(id); parent; parent = parents.at(*parent))
		depth++;
	return depth;
}

std::unique_ptr<TreeCard> TreePage::createCard(TreeItemId id) {
	auto card = std::make_unique<TreeCard>(id);
	auto controls = std::make_unique<CardControls>();
	controls->amount.setSizing(AxisSizing::fill(), AxisSizing::fit());
	controls->enabled.setChecked(id % 2 == 1);
	card->content().addChild(controls->amount);
	card->content().addChild(controls->enabled);
	mControls[id] = std::move(controls);
	return card;
}

void TreePage::updateCard(TreeCard& card) const {
	const size_t depth = model.depthOf(card.id());
	const halnf hue = halnf(depth) / halnf(kLevels);
	card.setTitle("Node " + std::to_string(card.id()));
	card.setLabel("#" + std::to_string(card.id()));
	card.setAccent(RGBA{ 0.30f + 0.45f * hue, 0.55f - 0.20f * hue, 0.75f - 0.35f * hue, 1.0f });
}

void TreePage::refreshStatus() {
	const std::optional<TreeItemId> selected = view.selection();
	status.setText(
		std::to_string(model.parents.size()) + " nodes, " + std::to_string(view.structure().height() + 1) + " levels" +
		(selected ? "  -  selected #" + std::to_string(*selected) : std::string())
	);
}

void NodesPage::refreshStatus() {
	status.setText(
		std::to_string(graph.nodes().size()) + " nodes, " + std::to_string(graph.links().size()) + " links, " +
		std::to_string(editor.selection().size()) + " selected"
	);
}

Gallery::Gallery() :
	Panel(Surface::Window) {
	setDebugName("gallery");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	emplaceLayout<ColumnLayout>();

	buildMenus();

	tabs.setSizing(AxisSizing::fill(), AxisSizing::fill());
	controlsScroll.setContent(&controls);
	tabs.addTab("Controls", controlsScroll);
	tabs.addTab("List", listPage);
	tabs.addTab("Canvas", canvasPage);
	tabs.addTab("Docking", dockPage);
	tabs.addTab("Input", inputPage);
	tabs.addTab("Reorder", reorderPage);
	tabs.addTab("Theme", themePage);
	tabs.addTab("Media", mediaPage);
	tabs.addTab("Files", filesPage);
	tabs.addTab("Nodes", nodesPage);
	tabs.addTab("Tree", treePage);
	tabs.addTab("Kanban", kanbanPage);
	tabs.addTab("Markdown", markdownPage);
	tabs.addTab("Carousel", carouselPage);

	statusBar.setTone(Label::Tone::Muted);
	statusBar.setText("ready");
	statusArea.setSizing(AxisSizing::fill(), AxisSizing::fit());
	statusArea.emplaceLayout<RowLayout>().setPadding(Insets::symmetric(10, 3));
	statusArea.addChild(statusBar);

	addChild(menuBar);
	addChild(tabs);
	addChild(statusArea);
}

void Gallery::onEvent(Event& event) {
	const auto* binding = shortcuts.find(event);
	if (!binding) return;

	run(binding->command);
	event.accept();
}

void Gallery::run(Action action) {
	switch (action) {
		case Action::New: report("new"); break;
		case Action::Open: report("open"); break;
		case Action::Quit: report("quit"); break;
		case Action::NextTab: tabs.setCurrentIndex((tabs.currentIndex() + 1) % tabs.tabCount()); break;
	}
}

void Gallery::buildMenus() {
	shortcuts.bind(ctrl(InputID::N), Action::New);
	shortcuts.bind(ctrl(InputID::O), Action::Open);
	shortcuts.bind(ctrl(InputID::Q), Action::Quit);
	shortcuts.bind(ctrl(InputID::TAB), Action::NextTab, true);

	fileMenu.addItem("New", [this]() { run(Action::New); }, shortcuts.textFor(Action::New));
	fileMenu.addItem("Open...", [this]() { run(Action::Open); }, shortcuts.textFor(Action::Open));
	fileMenu.addSubmenu("Open Recent", recentMenu);
	fileMenu.addSeparator();
	fileMenu.addItem("Quit", [this]() { run(Action::Quit); }, shortcuts.textFor(Action::Quit));

	recentMenu.addItem("sketch.sketch2d", [this]() { report("open sketch.sketch2d"); });
	recentMenu.addItem("portrait.sketch2d", [this]() { report("open portrait.sketch2d"); });

	viewMenu.addToggle(
		"Dark Mode", [this]() { return state.darkMode; }, [this]() { controls.darkMode.setChecked(!state.darkMode); }
	);
	MenuEntry lastAction = MenuEntry::item("Last action", {});
	lastAction.dynamicText = [this]() {
		return state.lastAction.empty() ? std::string("No action yet") : "Last action: " + state.lastAction;
	};
	lastAction.isEnabled = [this]() { return !state.lastAction.empty(); };
	viewMenu.add(lastAction);
	viewMenu.addSeparator();
	viewMenu.addItem("Controls", [this]() { tabs.setCurrentIndex(0); });
	viewMenu.addItem("List", [this]() { tabs.setCurrentIndex(1); });
	viewMenu.addItem("Canvas", [this]() { tabs.setCurrentIndex(2); });
	viewMenu.addItem("Input", [this]() { tabs.setCurrentIndex(4); });
	viewMenu.addItem("Markdown", [this]() { tabs.setCurrentIndex(tabs.tabCount() - 1); });
	viewMenu.addItem("Next Tab", [this]() { run(Action::NextTab); }, shortcuts.textFor(Action::NextTab));

	menuBar.addMenu("File", fileMenu);
	menuBar.addMenu("View", viewMenu);
}

void Gallery::report(const std::string& action) {
	state.lastAction = action;
	statusBar.setText(action);
}

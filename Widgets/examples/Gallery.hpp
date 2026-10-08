#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Dialog.hpp"
#include "FileBrowser.hpp"
#include "Docking.hpp"
#include "FloatingPanel.hpp"
#include "KanbanBoard.hpp"
#include "Label.hpp"
#include "MarkdownEditor.hpp"
#include "Menus.hpp"
#include "MediaControls.hpp"
#include "NodeEditor.hpp"
#include "Notice.hpp"
#include "ReorderableList.hpp"
#include "ThemeEditor.hpp"
#include "Shortcut.hpp"
#include "Slider.hpp"
#include "Collapsible.hpp"
#include "ColorPicker.hpp"
#include "TextArea.hpp"
#include "TextField.hpp"
#include "Toggles.hpp"
#include "TreeView.hpp"
#include "Views.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>

namespace tp::ui::gallery {

	struct GalleryState {
		int buttonClicks = 0;
		bool notificationsEnabled = true;
		bool darkMode = true;
		long qualityIndex = 1;
		halnf volume = 0.6f;
		RGBA brushColor = { 0.9f, 0.4f, 0.2f, 1.0f };
		std::string lastAction;
		std::string userName = "the reference app";
		halnf brushSize = 14;
		int confirmations = 0;
	};

	class InputPage : public Widget {
	public:
		explicit InputPage(GalleryState& state);

		Label nameLabel{ "Name" };
		TextField name;
		Label sizeLabel{ "Brush size" };
		NumberField size{ 1, 300, 14 };
		Label greeting;
		Label notesLabel{ "Notes (drag the bottom edge to resize)" };
		TextArea notes;
		Label colorLabel{ "Accent color" };
		ColorPickerButton color;
		Button openDialog{ "Show dialog" };
		Button askConfirm{ "Clear history..." };
		Button showNotice{ "Show notice" };
		Notice notice;
		Label confirmations;
		Dialog dialog{ "About" };
		ConfirmDialog confirm;

	private:
		void refresh();

		GalleryState& mState;
		Widget mDialogRow;
	};

	class ControlsPage : public Widget {
	public:
		explicit ControlsPage(GalleryState& state);

		Button clickMe{ "Click me" };
		Button accent{ "Accent" };
		Button disabled{ "Disabled" };
		Label clickCount;

		Checkbox notifications{ "Enable notifications" };
		Switch darkMode{ "Dark mode" };

		RadioButton low{ "Low" };
		RadioButton medium{ "Medium" };
		RadioButton high{ "High" };
		RadioGroup quality;

		LabeledSlider volume{ "Volume", 0, 1, 0.6f };
		LabeledSlider brushSize{ "Brush size (logarithmic)", 1, 1000, 10 };
		ProgressBar progress;
		Dropdown blendMode{ { "Normal", "Multiply", "Screen", "Overlay" } };
		Dropdown accentColor{ { "Blue", "Purple", "Green", "Orange", "Pink" } };

	private:
		void refreshClickCount();

		GalleryState& mState;
		Widget mButtonRow;
		Widget mRadioRow;
		Separator mFirstSeparator;
		Separator mSecondSeparator;
	};

	class ReorderPage : public Widget {
	public:
		ReorderPage();

		Label hint{ "Drag the dots, or a row, up or down to reorder. Esc cancels." };
		ReorderableList list;
		std::vector<std::unique_ptr<Checkbox>> tasks;
		Label status;

	private:
		void refreshStatus();
	};

	class FilesPage : public Widget {
	public:
		explicit FilesPage(FileSystem& fileSystem = FileSystem::local());

		Label hint{ "Browse folders, then open, save or choose a folder with the file dialog. Double-click enters a "
								"folder; Backspace goes up." };
		FileBrowser browser;
		Widget actions;
		Button openFile{ "Open file..." };
		Button saveFile{ "Save as..." };
		Button chooseFolder{ "Choose folder..." };
		Label status;
		FileDialog dialog;

	private:
		void report(const std::string& action, const std::string& path);
	};

	class MediaPage : public Widget {
	public:
		MediaPage();

		Label hint{ "A music player bar built from transport buttons, a seek bar and a volume control." };
		std::vector<NowPlaying> playlist;
		size_t current = 0;
		PlayerBar player;
		Label status;

	protected:
		void onTick(halnf deltaMs) override;

	private:
		void load(size_t index);
		void togglePlaying();
	};

	class ListPage : public Widget {
	public:
		ListPage();

		ScrollView scroll;
		Widget content;
		ReorderableList list{ 2 };
		std::vector<std::unique_ptr<Collapsible>> groups;
		std::vector<std::unique_ptr<Collapsible>> subgroups;
		std::vector<std::unique_ptr<Button>> groupRows;
		std::vector<std::unique_ptr<ReorderableList>> favoriteLists;
		Collapsible allRows{ "All rows" };
		std::vector<std::unique_ptr<Button>> rows;

		Collapsible draggableSection{ "Draggable list" };
		ReorderableList draggable;
		std::vector<std::unique_ptr<Checkbox>> draggableItems;
		Label draggableStatus;

		Collapsible staticSection{ "Static list" };
		Widget staticList;
		std::vector<std::unique_ptr<Checkbox>> staticItems;
	};

	class CanvasPage : public Widget {
	public:
		explicit CanvasPage(GalleryState& state);

		SplitView split{ Axis::Horizontal };
		Panel desk{ Panel::Surface::Window };
		FloatingPanel colorPanel{ "Brush color" };
		ColorWheel wheel;
		Panel inspector{ Panel::Surface::Base };
		Label inspectorTitle{ "Inspector" };
		Label colorReadout;

	private:
		void refreshReadout();

		GalleryState& mState;
	};

	class DockPage : public Widget {
	public:
		DockPage();

		DockArea dock;
		Panel editor{ Panel::Surface::Base };
		Label editorText{ "Main view: drag panel headers to dock them on another side" };

		DockPanel project{ "Project", DockSide::Left };
		DockPanel structure{ "Structure", DockSide::Left };
		DockPanel terminal{ "Terminal", DockSide::Bottom };
		DockPanel properties{ "Properties", DockSide::Right };

		Label projectText{ "src/  tests/  docs/" };
		Label structureText{ "class Gallery" };
		Label terminalText{ "$ cmake --build build" };
		Label propertiesText{ "width 240  height 120" };
	};

	class NodesPage : public Widget {
	public:
		NodesPage();

		Label hint{ "Drag from a port to connect, drag a linked input to detach. Drag empty space to select, Alt or "
								"middle drag to pan, wheel to zoom. Ctrl+C/X/V/D, Delete, Home." };
		NodeGraph graph;
		NodeEditor editor{ graph };
		Label status;

	private:
		void populate();
		void refreshStatus();
	};

	class CarouselPage : public Widget {
	public:
		CarouselPage();

		Label hint{ "A cyclic carousel: the arrows, the tabs, a horizontal swipe or Shift+wheel slide between pages. "
								"The wheel goes to the carousel only when nothing inside the page scrolls with it." };
		TabView carousel{ TabView::Style::Carousel };
		Label colorsText;
		Label notesText;
		ScrollView longListScroll;
		Widget longList;
		std::vector<std::unique_ptr<Label>> longListRows;
	};

	class KanbanPage : public Widget {
	public:
		KanbanPage();

		struct Story {
			std::string title;
			std::string detail;
			RGBA accent;
		};

		class Model : public KanbanModel {
		public:
			std::vector<std::vector<KanbanItemId>> columns;
			std::map<KanbanItemId, Story> stories;

			[[nodiscard]] std::vector<KanbanItemId> items(size_t column) const override { return columns[column]; }
			[[nodiscard]] bool canMove(const KanbanMove& move) const override;
			void apply(const KanbanMove& move);
		};

		Label hint{ "Drag cards between columns or within one. Review holds two cards; Released only takes cards "
								"from Review and keeps its own order. Esc cancels a drag, double-click opens a card." };
		Model model;
		KanbanBoard board{ model };
		Label status;

	private:
		void populate();
		void refreshStatus(const std::string& action);
	};

	class TreePage : public Widget {
	public:
		static constexpr size_t kLevels = 10;

		TreePage();

		class Model : public TreeModel {
		public:
			std::map<TreeItemId, std::optional<TreeItemId>> parents;

			[[nodiscard]] std::vector<TreeItemId> roots() const override;
			[[nodiscard]] std::vector<TreeItemId> children(TreeItemId id) const override;
			[[nodiscard]] size_t depthOf(TreeItemId id) const;
		};

		Label hint{ "A generated tree, ten levels deep. Wheel up/down or pinch zooms the camera; horizontal wheel "
								"or Shift+wheel opens and folds levels of the selected card, or of the whole tree without a "
								"selection. Tap a selected card to open or close it. Arrow keys walk the hierarchy, drag a "
								"children port to empty space to add a child, drag a parent port onto another card to move it." };
		Model model;
		TreeView view{ model };
		Label status;

	private:
		struct CardControls {
			Slider amount{ 0, 100, 50 };
			Checkbox enabled{ "Enabled" };
		};

		void populate();
		void refreshStatus();
		[[nodiscard]] std::unique_ptr<TreeCard> createCard(TreeItemId id);
		void updateCard(TreeCard& card) const;

		std::map<TreeItemId, std::unique_ptr<CardControls>> mControls;
	};

	class MarkdownPage : public Widget {
	public:
		static const char* const kSampleDocument;
		static const char* const kSampleDirectory;

		explicit MarkdownPage(FileSystem& fileSystem = FileSystem::local());

		Widget actions;
		Button openFile{ "Open..." };
		Button saveFile{ "Save as..." };
		Button reset{ "Reset sample" };
		Label status;
		MarkdownEditor editor;
		FileDialog dialog;

	private:
		void showSample();
		[[nodiscard]] std::optional<std::string> storePastedImage(const ClipboardImage& image);
		void copyPastedImages(const std::string& fromFolder, const std::string& toFolder);
		void open(const std::string& path);
		void save(const std::string& path);

		FileSystem& mFileSystem;
		std::string mDirectory;
		bool mOpenedFile = false;
	};

	class Gallery : public Panel {
	public:
		Gallery();

		GalleryState state;

		MenuBar menuBar;
		Menu fileMenu;
		Menu viewMenu;
		Menu recentMenu;
		TabView tabs;
		Panel statusArea{ Panel::Surface::None };
		Label statusBar;

		ControlsPage controls{ state };
		ScrollView controlsScroll;
		InputPage inputPage{ state };
		ListPage listPage;
		CanvasPage canvasPage{ state };
		DockPage dockPage;
		ReorderPage reorderPage;
		ThemeEditorPage themePage;
		MediaPage mediaPage;
		FilesPage filesPage;
		CarouselPage carouselPage;
		NodesPage nodesPage;
		TreePage treePage;
		KanbanPage kanbanPage;
		MarkdownPage markdownPage;

		enum class Action { New, Open, Quit, NextTab };
		ShortcutMap<Action> shortcuts;

	protected:
		void onEvent(Event& event) override;

	private:
		void buildMenus();
		void run(Action action);
		void report(const std::string& action);
	};
}

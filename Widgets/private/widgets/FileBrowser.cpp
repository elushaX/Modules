#include "FileBrowser.hpp"

#include "BoxLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kBrowserHeight = 300;
	constexpr halnf kFileDialogWidth = 520;
	constexpr halnf kRowHeight = 26;
	constexpr halnf kKindWidth = 18;

	class LocalFileSystem : public FileSystem {
	public:
		[[nodiscard]] std::optional<std::vector<FileEntry>> list(const std::string& directory) const override {
			std::error_code error;
			std::filesystem::directory_iterator iterator(
				directory, std::filesystem::directory_options::skip_permission_denied, error
			);
			if (error) return std::nullopt;

			std::vector<FileEntry> entries;
			for (const std::filesystem::directory_entry& item : iterator) {
				FileEntry entry;
				entry.name = item.path().filename().string();
				entry.isDirectory = item.is_directory(error);
				if (!entry.isDirectory) {
					const std::uintmax_t size = item.file_size(error);
					entry.size = error ? 0 : (uint64_t) size;
				}
				entries.push_back(std::move(entry));
			}
			return entries;
		}

		[[nodiscard]] bool isDirectory(const std::string& path) const override {
			std::error_code error;
			return std::filesystem::is_directory(path, error);
		}

		[[nodiscard]] bool exists(const std::string& path) const override {
			std::error_code error;
			return std::filesystem::exists(path, error);
		}

		bool createDirectory(const std::string& path) override {
			std::error_code error;
			return std::filesystem::create_directories(path, error) && !error;
		}

		[[nodiscard]] std::optional<std::string> readText(const std::string& path) const override {
			std::ifstream file(path, std::ios::binary);
			if (!file) return std::nullopt;

			std::ostringstream content;
			content << file.rdbuf();
			return content.str();
		}

		bool writeText(const std::string& path, const std::string& text) override {
			std::ofstream file(path, std::ios::binary | std::ios::trunc);
			return file && (file << text) && file.flush();
		}

		[[nodiscard]] std::string home() const override {
			const char* home = std::getenv("HOME");
			return home && *home ? home : "/";
		}
	};

	std::string lowered(std::string text) {
		for (char& character : text)
			character = (char) std::tolower((unsigned char) character);
		return text;
	}

	bool sortsBefore(const FileEntry& left, const FileEntry& right) {
		if (left.isDirectory != right.isDirectory) return left.isDirectory;
		const std::string a = lowered(left.name);
		const std::string b = lowered(right.name);
		return a == b ? left.name < right.name : a < b;
	}

	bool isValidName(const std::string& name) {
		return !name.empty() && name != "." && name != ".." && name.find('/') == std::string::npos;
	}
}

FileSystem& FileSystem::local() {
	static LocalFileSystem fileSystem;
	return fileSystem;
}

std::string tp::ui::joinPath(const std::string& directory, const std::string& name) {
	if (directory.empty()) return name;
	if (directory.back() == '/') return directory + name;
	return directory + "/" + name;
}

std::string tp::ui::parentPath(const std::string& path) {
	if (path.empty() || path == "/") return "/";
	std::string trimmed = path;
	while (trimmed.size() > 1 && trimmed.back() == '/')
		trimmed.pop_back();
	const size_t slash = trimmed.find_last_of('/');
	if (slash == std::string::npos) return ".";
	if (slash == 0) return "/";
	return trimmed.substr(0, slash);
}

bool tp::ui::createDirectories(FileSystem& files, const std::string& directory) {
	if (files.isDirectory(directory)) return true;
	const std::string parent = parentPath(directory);
	if (parent == directory || !createDirectories(files, parent)) return false;
	return files.createDirectory(directory) || files.isDirectory(directory);
}

std::string tp::ui::fileName(const std::string& path) {
	std::string trimmed = path;
	while (trimmed.size() > 1 && trimmed.back() == '/')
		trimmed.pop_back();
	const size_t slash = trimmed.find_last_of('/');
	return slash == std::string::npos ? trimmed : trimmed.substr(slash + 1);
}

std::string tp::ui::describeSize(uint64_t bytes) {
	const char* const units[] = { "B", "KB", "MB", "GB", "TB" };
	double value = (double) bytes;
	size_t unit = 0;
	while (value >= 1024.0 && unit + 1 < std::size(units)) {
		value /= 1024.0;
		unit++;
	}
	char text[32];
	if (unit == 0) std::snprintf(text, sizeof(text), "%llu B", (unsigned long long) bytes);
	else std::snprintf(text, sizeof(text), value < 10 ? "%.1f %s" : "%.0f %s", value, units[unit]);
	return text;
}

FileRow::FileRow(const FileEntry& entry, std::function<void(FileRow&)> onSelect, std::function<void(FileRow&)> onOpen) :
	Button(entry.name),
	mEntry(entry),
	mOnSelect(std::move(onSelect)),
	mOnOpen(std::move(onOpen)) {
	setDebugName("file row: " + entry.name);
	setStyle(Style::Flat);
	setSizing(AxisSizing::fill(), AxisSizing::fixed(kRowHeight));
}

void FileRow::onEvent(Event& event) {
	if (event.type() == Event::Type::Click && event.button() == PointerButton::Primary) {
		event.accept();
		if (mOnSelect) mOnSelect(*this);
		if (event.clickCount() >= 2 && mOnOpen) mOnOpen(*this);
		return;
	}
	if (event.type() == Event::Type::KeyDown && event.keyCode() == InputID::ENTER) {
		event.accept();
		if (mOnOpen) mOnOpen(*this);
		return;
	}
	Button::onEvent(event);
}

void FileRow::paint(Painter& painter) {
	const Theme& style = theme();
	const RectF area = localRect();
	painter.fillRect(area, background(), style.metrics.cornerRadius);
	if (showsFocusRing()) painter.strokeRect(area, style.palette.focusRing, style.metrics.cornerRadius, 2);

	const halnf fontSize = style.metrics.fontSize;
	const RGBA muted = style.palette.mutedText;
	const halnf pad = style.metrics.controlPadding.left;

	const RectF kind = makeRect(pad, 0, kKindWidth, area.size.y);
	if (mEntry.isDirectory) {
		const halnf top = area.size.y * 0.5f - 5;
		painter.fillRect(makeRect(kind.pos.x, top - 2, 7, 3), muted, 1);
		painter.strokeRect(makeRect(kind.pos.x, top, 14, 10), muted, 2, 1.25f);
	} else {
		const halnf top = area.size.y * 0.5f - 7;
		painter.strokeRect(makeRect(kind.pos.x + 2, top, 10, 14), muted, 1.5f, 1.25f);
	}

	const std::string size = mEntry.isDirectory ? std::string() : describeSize(mEntry.size);
	const halnf sizeWidth = size.empty() ? 0 : textWidth(size, fontSize) + pad;
	const halnf nameLeft = kind.pos.x + kKindWidth + pad * 0.5f;
	painter.text(
		mEntry.name,
		makeRect(nameLeft, 0, area.size.x - nameLeft - sizeWidth - pad, area.size.y),
		fontSize,
		TextAlign::Start,
		isSelected() ? style.palette.text : (mEntry.isHidden() ? muted : style.palette.text)
	);
	if (!size.empty())
		painter.text(
			size, makeRect(area.size.x - sizeWidth - pad, 0, sizeWidth, area.size.y), fontSize, TextAlign::End, muted
		);
}

FileBrowser::FileBrowser(FileSystem& fileSystem) :
	mFileSystem(fileSystem) {
	setDebugName("file browser");
	setSizing(AxisSizing::fill(), AxisSizing::fixed(kBrowserHeight));
	emplaceLayout<ColumnLayout>(6);

	mHeader.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mHeader.emplaceLayout<RowLayout>(6).setCrossAlign(CrossAlign::Center);
	up.setTooltip("Parent folder (Backspace)");
	up.setOnActivate([this]() { goUp(); });
	path.setOnPartChosen([this](size_t index) {
		const std::vector<std::string> directories = pathDirectories();
		if (index < directories.size()) setDirectory(directories[index]);
	});
	mHeader.addChild(up);
	mHeader.addChild(path);

	mList.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mList.emplaceLayout<ColumnLayout>(1);
	mEmpty.setTone(Label::Tone::Muted);
	scroll.setSizing(AxisSizing::fill(), AxisSizing::fill());
	scroll.setContent(&mList);

	addChild(mHeader);
	addChild(scroll);
}

bool FileBrowser::setDirectory(const std::string& directory) {
	std::optional<std::vector<FileEntry>> listed = mFileSystem.list(directory);
	if (!listed) {
		mError = "Can't open " + directory;
		return false;
	}

	mError.clear();
	const bool changed = directory != mDirectory;
	mDirectory = directory;
	mEntries = std::move(*listed);
	std::sort(mEntries.begin(), mEntries.end(), sortsBefore);
	if (changed) mSelected.clear();
	refreshPath();
	up.setEnabled(!isAtRoot());
	rebuildRows();
	if (changed) scroll.jumpTo(0);
	if (changed && onDirectoryChanged) onDirectoryChanged(mDirectory);
	return true;
}

bool FileBrowser::isAtRoot() const { return mDirectory.empty() || mDirectory == "/" || mDirectory == mRootDirectory; }

void FileBrowser::setRootDirectory(std::string root) {
	mRootDirectory = root.empty() ? std::string("/") : std::move(root);
	up.setEnabled(!isAtRoot());
	refreshPath();
}

bool FileBrowser::isInsideRoot(const std::string& directory) const {
	if (mRootDirectory == "/" || directory == mRootDirectory) return true;
	return directory.rfind(mRootDirectory + "/", 0) == 0;
}

std::vector<std::string> FileBrowser::pathDirectories() const {
	std::vector<std::string> directories;
	if (mDirectory.empty()) return directories;

	const std::string top = isInsideRoot(mDirectory) ? mRootDirectory : std::string("/");
	std::string directory = mDirectory;
	while (directory != top) {
		directories.push_back(directory);
		const std::string parent = parentPath(directory);
		if (parent == directory) break;
		directory = parent;
	}
	if (directory == top) directories.push_back(top);
	std::reverse(directories.begin(), directories.end());
	return directories;
}

void FileBrowser::refreshPath() {
	const std::vector<std::string> directories = pathDirectories();
	std::vector<std::string> names;
	for (const std::string& directory : directories) {
		const std::string name = fileName(directory);
		names.push_back(name.empty() ? directory : name);
	}
	path.setParts(names);
	for (size_t index = 0; index < directories.size(); index++)
		path.part(index).setTooltip(directories[index]);
}

bool FileBrowser::goUp() {
	if (isAtRoot()) return false;
	const std::string child = fileName(mDirectory);
	if (!setDirectory(parentPath(mDirectory))) return false;
	select(child);
	return true;
}

void FileBrowser::refresh() {
	if (!mDirectory.empty()) setDirectory(mDirectory);
}

void FileBrowser::setExtensions(std::vector<std::string> extensions) {
	mExtensions.clear();
	for (std::string& extension : extensions)
		mExtensions.push_back(lowered(extension));
	rebuildRows();
}

void FileBrowser::setShowsFiles(bool showsFiles) {
	mShowsFiles = showsFiles;
	rebuildRows();
}

void FileBrowser::setShowsHidden(bool showsHidden) {
	mShowsHidden = showsHidden;
	rebuildRows();
}

bool FileBrowser::isListed(const FileEntry& entry) const {
	if (entry.isHidden() && !mShowsHidden) return false;
	if (entry.isDirectory) return true;
	if (!mShowsFiles) return false;
	if (mExtensions.empty()) return true;
	const std::string name = lowered(entry.name);
	return std::any_of(mExtensions.begin(), mExtensions.end(), [&](const std::string& extension) {
		return name.size() >= extension.size() &&
					 name.compare(name.size() - extension.size(), extension.size(), extension) == 0;
	});
}

void FileBrowser::rebuildRows() {
	mList.removeAllChildren();
	mRows.clear();
	for (const FileEntry& entry : mEntries) {
		if (!isListed(entry)) continue;
		auto row = std::make_unique<FileRow>(
			entry, [this](FileRow& chosen) { select(chosen.entry().name); }, [this](FileRow& chosen) { open(chosen.entry()); }
		);
		row->setSelected(entry.name == mSelected);
		mList.addChild(*row);
		mRows.push_back(std::move(row));
	}
	if (mRows.empty()) mList.addChild(mEmpty);
}

FileRow* FileBrowser::row(const std::string& name) const {
	for (const auto& row : mRows) {
		if (row->entry().name == name) return row.get();
	}
	return nullptr;
}

const FileEntry* FileBrowser::selected() const {
	const FileRow* found = row(mSelected);
	return found ? &found->entry() : nullptr;
}

void FileBrowser::select(const std::string& name) {
	mSelected = name;
	for (const auto& row : mRows)
		row->setSelected(row->entry().name == name);
	const FileEntry* entry = selected();
	if (entry && onSelected) onSelected(*entry);
}

void FileBrowser::open(const FileEntry& entry) {
	const std::string target = joinPath(mDirectory, entry.name);
	if (entry.isDirectory) {
		setDirectory(target);
		return;
	}
	if (onFileOpened) onFileOpened(target);
}

void FileBrowser::onEvent(Event& event) {
	if (event.type() != Event::Type::KeyDown || event.keyCode() != InputID::BACKSPACE) return;
	if (ui() && dynamic_cast<TextField*>(ui()->keyboardFocus())) return;
	event.accept();
	goUp();
}

FileDialog::FileDialog(FileSystem& fileSystem) :
	Dialog("Choose file"),
	browser(fileSystem) {
	setDebugName("file dialog");
	setSizing(AxisSizing::fixed(kFileDialogWidth), AxisSizing::fit());

	name.setPlaceholder("Name");
	name.setOnCommitted([this](const std::string&) {
		if (isOpen() && mCommitFromEnter) confirm();
	});
	problem.setTone(Label::Tone::Muted);
	problem.setVisible(false);

	browser.onSelected = [this](const FileEntry& entry) { syncNameFromSelection(entry); };
	browser.onFileOpened = [this](const std::string& path) {
		if (mMode == Mode::Open) finish(path);
		if (mMode == Mode::Save) {
			name.setText(fileName(path));
			confirm();
		}
	};
	browser.onDirectoryChanged = [this](const std::string&) { problem.setVisible(false); };

	hidden.setOnChanged([this](bool checked) { browser.setShowsHidden(checked); });
	newFolder.setOnActivate([this]() { createFolder(); });

	mOptions.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mOptions.emplaceLayout<RowLayout>(8);
	mOptions.addChild(newFolder);
	mOptions.addChild(hidden);

	body().addChild(browser);
	body().addChild(mOptions);
	body().addChild(name);
	body().addChild(problem);

	mConfirm = &addButton("Open", {}, false);
	mConfirm->setStyle(Button::Style::Accent);
	mConfirm->setOnActivate([this]() { confirm(); });
	addCancelButton();
}

void FileDialog::filterDescendantEvent(Event& event) {
	const bool enter =
		event.type() == Event::Type::KeyDown && (event.keyCode() == InputID::ENTER || event.keyCode() == InputID::KP_ENTER);
	mCommitFromEnter = enter && event.target() == &name;
}

void FileDialog::onEvent(Event& event) {
	if (event.type() != Event::Type::KeyDown) return;
	if (event.keyCode() != InputID::ENTER && event.keyCode() != InputID::KP_ENTER) return;
	if (ui() && dynamic_cast<FileRow*>(ui()->keyboardFocus())) return;
	event.accept();
	confirm();
}

void FileDialog::show(
	Ui& ui,
	Mode mode,
	std::string title,
	const std::string& directory,
	const std::string& suggestedName,
	std::function<void(const std::string&)> onChosen
) {
	mMode = mode;
	mOnChosen = std::move(onChosen);
	setTitle(std::move(title));
	problem.setVisible(false);
	name.setText(suggestedName);
	name.setVisible(mode != Mode::ChooseDirectory);
	browser.setShowsFiles(mode != Mode::ChooseDirectory);
	mConfirm->setText(mode == Mode::Save ? "Save" : (mode == Mode::Open ? "Open" : "Choose"));

	FileSystem& fileSystem = browser.fileSystem();
	std::string start = directory;
	while (!start.empty() && start != "/" && !fileSystem.isDirectory(start))
		start = parentPath(start);
	if (start.empty() || !browser.setDirectory(start)) browser.setDirectory(fileSystem.home());

	open(ui);
	if (mode == Mode::Save) {
		ui.setKeyboardFocus(&name);
		name.selectAll();
	}
}

std::string FileDialog::chosenPath() const {
	if (mMode == Mode::ChooseDirectory) {
		const FileEntry* entry = browser.selected();
		return entry && entry->isDirectory ? joinPath(browser.directory(), entry->name) : browser.directory();
	}
	const std::string& typed = name.text();
	if (typed.empty()) return {};
	if (typed.front() == '/') return typed;
	return joinPath(browser.directory(), typed);
}

void FileDialog::confirm() {
	const std::string path = chosenPath();
	FileSystem& fileSystem = browser.fileSystem();

	if (mMode != Mode::ChooseDirectory) {
		const std::string typed = name.text();
		if (typed.empty() || typed.back() == '/') {
			problem.setText("Enter a name");
			problem.setVisible(true);
			return;
		}
	}
	if (mMode != Mode::ChooseDirectory && fileSystem.isDirectory(path)) {
		browser.setDirectory(path);
		name.setText({});
		return;
	}
	if (mMode == Mode::Open && !fileSystem.exists(path)) {
		problem.setText(fileName(path) + " does not exist");
		problem.setVisible(true);
		return;
	}
	if (mMode == Mode::Save && fileSystem.exists(path) && ui()) {
		Ui& host = *ui();
		overwrite.ask(
			host,
			"Replace " + fileName(path) + "?",
			"It already exists in " + parentPath(path) + ".",
			"Replace",
			[this, path]() { finish(path); }
		);
		return;
	}
	finish(path);
}

void FileDialog::finish(const std::string& path) {
	close();
	auto chosen = std::move(mOnChosen);
	mOnChosen = {};
	if (chosen) chosen(path);
}

void FileDialog::createFolder() {
	FileSystem& fileSystem = browser.fileSystem();
	std::string folder = "New folder";
	for (int attempt = 2; fileSystem.exists(joinPath(browser.directory(), folder)); attempt++)
		folder = "New folder " + std::to_string(attempt);

	const std::string typed = name.text();
	if (mMode == Mode::ChooseDirectory && isValidName(typed)) folder = typed;
	if (!fileSystem.createDirectory(joinPath(browser.directory(), folder))) {
		problem.setText("Can't create " + folder);
		problem.setVisible(true);
		return;
	}
	browser.refresh();
	browser.select(folder);
}

void FileDialog::syncNameFromSelection(const FileEntry& entry) {
	problem.setVisible(false);
	if (entry.isDirectory || mMode == Mode::ChooseDirectory) return;
	name.setText(entry.name);
}

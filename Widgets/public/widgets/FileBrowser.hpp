#pragma once

#include "Breadcrumb.hpp"
#include "Button.hpp"
#include "Containers.hpp"
#include "Dialog.hpp"
#include "Label.hpp"
#include "TextField.hpp"
#include "Toggles.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tp::ui {

	struct FileEntry {
		std::string name;
		bool isDirectory = false;
		uint64_t size = 0;

		[[nodiscard]] bool isHidden() const { return !name.empty() && name.front() == '.'; }
		[[nodiscard]] bool operator==(const FileEntry& other) const = default;
	};

	class FileSystem {
	public:
		virtual ~FileSystem() = default;

		[[nodiscard]] virtual std::optional<std::vector<FileEntry>> list(const std::string& directory) const = 0;
		[[nodiscard]] virtual bool isDirectory(const std::string& path) const = 0;
		[[nodiscard]] virtual bool exists(const std::string& path) const = 0;
		virtual bool createDirectory(const std::string& path) = 0;
		[[nodiscard]] virtual std::optional<std::string> readText(const std::string& path) const = 0;
		virtual bool writeText(const std::string& path, const std::string& text) = 0;
		[[nodiscard]] virtual std::string home() const = 0;

		[[nodiscard]] static FileSystem& local();
	};

	[[nodiscard]] std::string joinPath(const std::string& directory, const std::string& name);
	[[nodiscard]] std::string parentPath(const std::string& path);
	bool createDirectories(FileSystem& files, const std::string& directory);
	[[nodiscard]] std::string fileName(const std::string& path);
	[[nodiscard]] std::string describeSize(uint64_t bytes);

	class FileRow : public Button {
	public:
		FileRow(const FileEntry& entry, std::function<void(FileRow&)> onSelect, std::function<void(FileRow&)> onOpen);

		[[nodiscard]] const FileEntry& entry() const { return mEntry; }

	protected:
		void onEvent(Event& event) override;
		void paint(Painter& painter) override;

	private:
		FileEntry mEntry;
		std::function<void(FileRow&)> mOnSelect;
		std::function<void(FileRow&)> mOnOpen;
	};

	class FileBrowser : public Widget {
	public:
		explicit FileBrowser(FileSystem& fileSystem = FileSystem::local());

		bool setDirectory(const std::string& directory);
		[[nodiscard]] const std::string& directory() const { return mDirectory; }
		bool goUp();
		void refresh();

		void setRootDirectory(std::string root);
		[[nodiscard]] const std::string& rootDirectory() const { return mRootDirectory; }

		void setExtensions(std::vector<std::string> extensions);
		void setShowsFiles(bool showsFiles);
		void setShowsHidden(bool showsHidden);
		[[nodiscard]] bool showsHidden() const { return mShowsHidden; }

		[[nodiscard]] const std::vector<FileEntry>& entries() const { return mEntries; }
		[[nodiscard]] const std::vector<std::unique_ptr<FileRow>>& rows() const { return mRows; }
		[[nodiscard]] FileRow* row(const std::string& name) const;
		[[nodiscard]] const FileEntry* selected() const;
		void select(const std::string& name);

		[[nodiscard]] FileSystem& fileSystem() const { return mFileSystem; }
		[[nodiscard]] const std::string& error() const { return mError; }

		[[nodiscard]] std::vector<std::string> pathDirectories() const;

		GlyphButton up{ GlyphButton::Glyph::Up };
		Breadcrumb path;
		ScrollView scroll;

		std::function<void(const FileEntry&)> onSelected;
		std::function<void(const std::string& path)> onFileOpened;
		std::function<void(const std::string& directory)> onDirectoryChanged;

	protected:
		void onEvent(Event& event) override;

	private:
		[[nodiscard]] bool isListed(const FileEntry& entry) const;
		[[nodiscard]] bool isAtRoot() const;
		[[nodiscard]] bool isInsideRoot(const std::string& directory) const;
		void rebuildRows();
		void refreshPath();
		void open(const FileEntry& entry);

		FileSystem& mFileSystem;
		std::string mDirectory;
		std::string mRootDirectory = "/";
		std::string mError;
		std::vector<std::string> mExtensions;
		bool mShowsFiles = true;
		bool mShowsHidden = false;
		std::vector<FileEntry> mEntries;
		std::vector<std::unique_ptr<FileRow>> mRows;
		std::string mSelected;
		Widget mHeader;
		Widget mList;
		Label mEmpty{ "This folder is empty" };
	};

	class FileDialog : public Dialog {
	public:
		enum class Mode { Open, Save, ChooseDirectory };

		explicit FileDialog(FileSystem& fileSystem = FileSystem::local());

		void show(
			Ui& ui,
			Mode mode,
			std::string title,
			const std::string& directory,
			const std::string& suggestedName,
			std::function<void(const std::string& path)> onChosen
		);

		[[nodiscard]] Mode mode() const { return mMode; }
		[[nodiscard]] std::string chosenPath() const;
		void confirm();

		FileBrowser browser;
		TextField name;
		Button newFolder{ "New folder" };
		Checkbox hidden{ "Show hidden" };
		Label problem;
		ConfirmDialog overwrite;

	protected:
		void onEvent(Event& event) override;
		void filterDescendantEvent(Event& event) override;

	private:
		void finish(const std::string& path);
		void createFolder();
		void syncNameFromSelection(const FileEntry& entry);

		Mode mMode = Mode::Open;
		std::function<void(const std::string&)> mOnChosen;
		Widget mOptions;
		Button* mConfirm = nullptr;
		bool mCommitFromEnter = false;
	};
}

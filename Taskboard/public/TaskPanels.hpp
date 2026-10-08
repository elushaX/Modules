#pragma once

#include "FileBrowser.hpp"
#include "Label.hpp"
#include "MarkdownEditor.hpp"
#include "PageStack.hpp"
#include "TaskBoard.hpp"
#include "TaskHeader.hpp"
#include "TaskStore.hpp"

#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace tp::taskboard {

	class TaskDescriptionPanel : public ui::Widget {
	public:
		explicit TaskDescriptionPanel(TaskBoard& board);

		void showTask(std::optional<ui::NodeId> id);
		[[nodiscard]] std::optional<ui::NodeId> shownTask() const { return mShown; }

		ui::Label hint{ "Open a task to see and edit its description." };
		TaskHeader header;
		ui::MarkdownEditor editor;

		void setOnChanged(std::function<void()> callback);
		void setOnEditingChanged(EditingCallback callback);

	private:
		void buildLayout();
		void bindEditors();
		void change(const std::function<void(Task&)>& edit);
		void reportEditing(TaskField field, bool editing);
		void refreshFields(const Task& task);

		TaskBoard& mBoard;
		std::optional<ui::NodeId> mShown;
		bool mLoading = false;
		std::function<void()> mOnChanged;
		EditingCallback mOnEditingChanged;
	};

	class TaskWorkspacePanel : public ui::Widget {
	public:
		explicit TaskWorkspacePanel(TaskBoard& board, ui::FileSystem& files = ui::FileSystem::local());

		void showTask(std::optional<ui::NodeId> id);
		void openDocument(const std::string& path);
		[[nodiscard]] const std::string& workspace() const { return mWorkspace; }
		[[nodiscard]] const std::string& openedDocument() const { return mDocument; }

		ui::PageStack pages;
		ui::Label hint;
		ui::FileBrowser browser;
		ui::MarkdownEditor document;
		ui::Label documentStatus;

	private:
		void showWorkspace(const std::string& workspace);
		void showBrowser();
		void saveDocument(const std::string& text);

		TaskBoard& mBoard;
		ui::FileSystem& mFiles;
		std::string mWorkspace;
		std::string mDocument;
		ui::Widget mBrowserPage;
		ui::Widget mDocumentPage;
	};
}

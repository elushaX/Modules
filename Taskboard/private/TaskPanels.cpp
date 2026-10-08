#include "TaskPanels.hpp"

#include "BoxLayout.hpp"

#include <algorithm>
#include <cctype>

using namespace tp;
using namespace tp::taskboard;
using namespace tp::ui;

namespace {
	constexpr halnf kPadding = 12;
	constexpr halnf kGap = 6;
	constexpr const char* kMarkdownExtension = ".md";
	constexpr const char* kWorkdirTitle = "Workdir";
	constexpr const char* kNoTaskHint = "Open a task to browse the files its agent produced.";
	constexpr const char* kNoWorkdirHint = "This task has no workdir yet. It appears once its agent session starts.";

	bool isMarkdownFile(const std::string& path) {
		std::string lowered = path;
		std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
			return char(std::tolower(c));
		});
		const std::string extension = kMarkdownExtension;
		return lowered.size() > extension.size() &&
					 lowered.compare(lowered.size() - extension.size(), extension.size(), extension) == 0;
	}

	std::string relativeTo(const std::string& root, const std::string& path) {
		if (path.rfind(root, 0) != 0) return path;
		std::string relative = path.substr(root.size());
		while (!relative.empty() && relative.front() == '/')
			relative.erase(relative.begin());
		return relative;
	}

	void mutedHint(Label& label) {
		label.setTone(Label::Tone::Muted);
		label.setWrap(true);
		label.setSizing(AxisSizing::fill(), AxisSizing::fit());
	}
}

TaskDescriptionPanel::TaskDescriptionPanel(TaskBoard& board) :
	header(board),
	mBoard(board) {
	setDebugName("task description panel");
	buildLayout();
	bindEditors();
	showTask(std::nullopt);
}

void TaskDescriptionPanel::buildLayout() {
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	emplaceLayout<ColumnLayout>(kGap).setPadding(Insets::uniform(kPadding));

	mutedHint(hint);
	editor.sourceArea().setPlaceholder("What should the agent do? Markdown is supported.");
	editor.setMode(MarkdownEditor::Mode::Preview);

	addChild(hint);
	addChild(header);
	addChild(editor);
}

void TaskDescriptionPanel::setOnChanged(std::function<void()> callback) {
	mOnChanged = callback;
	header.setOnChanged(std::move(callback));
}

void TaskDescriptionPanel::setOnEditingChanged(EditingCallback callback) {
	mOnEditingChanged = callback;
	header.setOnEditingChanged(std::move(callback));
}

void TaskDescriptionPanel::bindEditors() {
	editor.setOnChanged([this](const std::string& text) { change([&](Task& task) { task.description = text; }); });
	editor.sourceArea().setOnEditingStarted([this] { reportEditing(TaskField::Description, true); });
	editor.sourceArea().setOnCommitted([this](const std::string&) { reportEditing(TaskField::Description, false); });
}

void TaskDescriptionPanel::reportEditing(TaskField field, bool editing) {
	if (!mOnEditingChanged) return;
	if (!editing) {
		mOnEditingChanged(std::nullopt);
		return;
	}
	if (mShown) mOnEditingChanged(std::make_pair(*mShown, field));
}

void TaskDescriptionPanel::change(const std::function<void(Task&)>& edit) {
	Task* task = mShown ? mBoard.task(*mShown) : nullptr;
	if (!task || mLoading) return;

	edit(*task);
	if (mOnChanged) mOnChanged();
}

void TaskDescriptionPanel::showTask(std::optional<NodeId> id) {
	mShown = id;
	const Task* task = id ? mBoard.task(*id) : nullptr;

	hint.setVisible(task == nullptr);
	header.setVisible(task != nullptr);
	header.showTask(id);
	editor.setVisible(task != nullptr);
	if (task) refreshFields(*task);
}

void TaskDescriptionPanel::refreshFields(const Task& task) {
	const bool wasLoading = mLoading;
	mLoading = true;
	if (!editor.sourceArea().isEditing() && editor.markdown() != task.description) editor.setMarkdown(task.description);
	mLoading = wasLoading;
}

TaskWorkspacePanel::TaskWorkspacePanel(TaskBoard& board, FileSystem& files) :
	browser(files),
	mBoard(board),
	mFiles(files) {
	setDebugName("task workspace panel");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	emplaceLayout<ColumnLayout>();

	pages.setSizing(AxisSizing::fill(), AxisSizing::fill());
	pages.setPagePadding(Insets::uniform(kPadding));
	pages.setOnPageShown([this](StackPage&) {
		if (pages.depth() == 1) mDocument.clear();
	});

	mutedHint(hint);
	browser.setSizing(AxisSizing::fill(), AxisSizing::fill());
	browser.setExtensions({ kMarkdownExtension });
	browser.onFileOpened = [this](const std::string& path) { openDocument(path); };
	mBrowserPage.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mBrowserPage.emplaceLayout<ColumnLayout>(kGap);
	mBrowserPage.addChild(hint);
	mBrowserPage.addChild(browser);

	document.setMode(MarkdownEditor::Mode::Preview);
	document.setOnChanged([this](const std::string& text) { saveDocument(text); });
	mutedHint(documentStatus);
	documentStatus.setVisible(false);
	mDocumentPage.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mDocumentPage.emplaceLayout<ColumnLayout>(kGap);
	mDocumentPage.addChild(documentStatus);
	mDocumentPage.addChild(document);

	addChild(pages);
	showBrowser();
	showTask(std::nullopt);
}

void TaskWorkspacePanel::showBrowser() {
	pages.clear();
	mDocument.clear();
	StackPage& root = pages.push(kWorkdirTitle, mBrowserPage);
	root.setScrollsContent(false);
	root.header().setVisible(false);
}

void TaskWorkspacePanel::showTask(std::optional<NodeId> id) {
	const Task* task = id ? mBoard.task(*id) : nullptr;
	if (!task) {
		mWorkspace.clear();
		hint.setText(kNoTaskHint);
		hint.setVisible(true);
		browser.setVisible(false);
		if (pages.depth() > 1) showBrowser();
		return;
	}
	showWorkspace(task->workspace);
}

void TaskWorkspacePanel::showWorkspace(const std::string& workspace) {
	const bool changed = workspace != mWorkspace;
	mWorkspace = workspace;
	if (changed && pages.depth() > 1) showBrowser();

	bool listed = false;
	if (!workspace.empty() && changed) {
		browser.setRootDirectory(workspace);
		listed = browser.setDirectory(workspace);
	} else if (!workspace.empty()) {
		browser.refresh();
		listed = browser.error().empty();
	}

	hint.setText(kNoWorkdirHint);
	hint.setVisible(!listed);
	browser.setVisible(listed);
}

void TaskWorkspacePanel::openDocument(const std::string& path) {
	if (!isMarkdownFile(path)) return;

	const std::optional<std::string> text = mFiles.readText(path);
	documentStatus.setVisible(!text);
	documentStatus.setText(text ? std::string() : "Can't read " + path);
	document.setBaseDirectory(parentPath(path));
	document.setMarkdown(text.value_or(std::string()));
	document.setMode(MarkdownEditor::Mode::Preview);
	mDocument = path;

	StackPage& page = pages.push(relativeTo(mWorkspace, path), mDocumentPage);
	page.setScrollsContent(false);
}

void TaskWorkspacePanel::saveDocument(const std::string& text) {
	if (mDocument.empty()) return;
	if (mFiles.writeText(mDocument, text)) {
		documentStatus.setVisible(false);
		return;
	}
	documentStatus.setText("Can't save " + mDocument);
	documentStatus.setVisible(true);
}

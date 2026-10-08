#include "SkillsPanel.hpp"

#include "BoxLayout.hpp"

namespace tp::taskboard {

	using namespace ui;

	namespace {
		constexpr halnf kPadding = 10;
		constexpr halnf kGap = 8;
		constexpr const char* kMarkdownExtension = ".md";
		constexpr const char* kSkillFile = "SKILL.md";
		constexpr const char* kLibraryTitle = "Skills";

		void mutedHint(Label& label) {
			label.setTone(Label::Tone::Muted);
			label.setSizing(AxisSizing::fill(), AxisSizing::fit());
		}

		bool isMarkdownFile(const std::string& path) {
			const std::string extension = kMarkdownExtension;
			return path.size() >= extension.size() && path.compare(path.size() - extension.size(), extension.size(), extension) == 0;
		}

		std::string skillTemplate(const std::string& name) {
			return "---\nname: " + name +
						 "\ndescription: >-\n  One-line summary of what this skill does and when an agent should load it.\n---\n\n# " +
						 name + "\n\n## Required secrets\n\nList the secret keys this skill needs (names only); fetch each value at run time with get_secret.\n\n## Instructions\n\n";
		}
	}

	SkillsPanel::SkillsPanel(std::string skillsDirectory, FileSystem& files) :
		browser(files),
		mFiles(files),
		mRoot(std::move(skillsDirectory)) {
		setDebugName("skills panel");
		setSizing(AxisSizing::fill(), AxisSizing::fill());
		emplaceLayout<ColumnLayout>();

		mPages.setSizing(AxisSizing::fill(), AxisSizing::fill());
		mPages.setPagePadding(Insets::uniform(kPadding));
		mPages.setOnPageShown([this](StackPage&) {
			if (mPages.depth() == 1) mDocument.clear();
		});

		mutedHint(mHint);
		mHint.setText("Each skill is a folder with a SKILL.md. Open one to edit it; edits are saved to disk and versioned in git.");

		mNewSkill.setStyle(Button::Style::Accent);
		mNewSkill.setSizing(AxisSizing::fit(), AxisSizing::fit());
		mNewSkill.setOnClicked([this](const Event&) { promptNewSkill(); });
		mToolbar.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mToolbar.emplaceLayout<RowLayout>(kGap);
		mToolbar.addChild(mHint);
		mToolbar.addChild(mNewSkill);

		browser.setSizing(AxisSizing::fill(), AxisSizing::fill());
		browser.setExtensions({ kMarkdownExtension });
		browser.setRootDirectory(mRoot);
		browser.onFileOpened = [this](const std::string& path) { openDocument(path); };

		mLibraryPage.setSizing(AxisSizing::fill(), AxisSizing::fill());
		mLibraryPage.emplaceLayout<ColumnLayout>(kGap);
		mLibraryPage.addChild(mToolbar);
		mLibraryPage.addChild(browser);

		document.setMode(MarkdownEditor::Mode::Split);
		document.setOnChanged([this](const std::string& text) { saveDocument(text); });
		mutedHint(documentStatus);
		documentStatus.setVisible(false);
		mDocumentPage.setSizing(AxisSizing::fill(), AxisSizing::fill());
		mDocumentPage.emplaceLayout<ColumnLayout>(kGap);
		mDocumentPage.addChild(documentStatus);
		mDocumentPage.addChild(document);

		mNewSkillDialog.setTitle("New skill");
		mNewSkillDialog.setMessage("Folder name for the skill (letters, digits, '-', '_').");
		mNewSkillName.setPlaceholder("my-skill");
		mNewSkillName.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mNewSkillError.setTone(Label::Tone::Warning);
		mNewSkillError.setVisible(false);
		mNewSkillDialog.body().setSizing(AxisSizing::fill(), AxisSizing::fit());
		mNewSkillDialog.body().emplaceLayout<ColumnLayout>(kGap);
		mNewSkillDialog.body().addChild(mNewSkillName);
		mNewSkillDialog.body().addChild(mNewSkillError);
		mNewSkillDialog.addButton("Create", [this] {
			if (createSkill(mNewSkillName.text())) mNewSkillDialog.close();
		}, true);
		mNewSkillDialog.addCancelButton();

		addChild(mPages);
		showLibrary();
		refresh();
	}

	void SkillsPanel::showLibrary() {
		mPages.clear();
		mDocument.clear();
		StackPage& root = mPages.push(kLibraryTitle, mLibraryPage);
		root.setScrollsContent(false);
		root.header().setVisible(false);
	}

	void SkillsPanel::refresh() {
		const bool listed = browser.setDirectory(browser.directory().empty() ? mRoot : browser.directory());
		browser.setVisible(listed);
		mHint.setVisible(true);
		if (!listed) mHint.setText("Skills folder is unavailable: " + mRoot);
	}

	void SkillsPanel::openDocument(const std::string& path) {
		if (!isMarkdownFile(path)) return;

		const std::optional<std::string> text = mFiles.readText(path);
		documentStatus.setVisible(!text);
		documentStatus.setText(text ? std::string() : "Can't read " + path);
		document.setBaseDirectory(parentPath(path));
		document.setMarkdown(text.value_or(std::string()));
		document.setMode(MarkdownEditor::Mode::Split);
		mDocument = path;

		StackPage& page = mPages.push(fileName(parentPath(path)) + "/" + fileName(path), mDocumentPage);
		page.setScrollsContent(false);
	}

	void SkillsPanel::saveDocument(const std::string& text) {
		if (mDocument.empty()) return;
		if (mFiles.writeText(mDocument, text)) {
			documentStatus.setVisible(false);
			return;
		}
		documentStatus.setText("Can't save " + mDocument);
		documentStatus.setVisible(true);
	}

	void SkillsPanel::promptNewSkill() {
		if (!ui()) return;
		mNewSkillName.setText("");
		mNewSkillError.setVisible(false);
		mNewSkillDialog.open(*ui(), this);
	}

	bool SkillsPanel::createSkill(const std::string& name) {
		const bool valid = !name.empty() && name.find('/') == std::string::npos && name.find('\\') == std::string::npos &&
												name.front() != '.';
		if (!valid) {
			mNewSkillError.setText("Use a folder name without slashes, not starting with a dot.");
			mNewSkillError.setVisible(true);
			return false;
		}

		const std::string folder = joinPath(mRoot, name);
		if (mFiles.exists(folder)) {
			mNewSkillError.setText("A skill named " + name + " already exists.");
			mNewSkillError.setVisible(true);
			return false;
		}
		if (!createDirectories(mFiles, folder) || !mFiles.writeText(joinPath(folder, kSkillFile), skillTemplate(name))) {
			mNewSkillError.setText("Could not create the skill folder.");
			mNewSkillError.setVisible(true);
			return false;
		}

		refresh();
		openDocument(joinPath(folder, kSkillFile));
		return true;
	}
}

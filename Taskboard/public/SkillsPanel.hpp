#pragma once

#include "Button.hpp"
#include "Dialog.hpp"
#include "FileBrowser.hpp"
#include "Label.hpp"
#include "MarkdownEditor.hpp"
#include "PageStack.hpp"
#include "TextField.hpp"

#include <string>

namespace tp::taskboard {

	// The skills library: a file browser over the git-versioned skills folder and an editor for the SKILL.md
	// inside each skill. Edits are written straight to disk, where git tracks every revision. Skills are the
	// place reusable instructions live - Elvees, self-hosting, and task-agnostic packs alike - so they can be
	// reviewed and edited here and loaded by agents through the server's skill tools.
	class SkillsPanel : public ui::Widget {
	public:
		explicit SkillsPanel(std::string skillsDirectory, ui::FileSystem& files = ui::FileSystem::local());

		void refresh();

		// Create a skill folder with a starter SKILL.md and open it. Returns false if the name is invalid or the
		// folder already exists. The New skill dialog calls this; tests drive it directly.
		bool createSkill(const std::string& name);

		ui::FileBrowser browser;
		ui::MarkdownEditor document;
		ui::Label documentStatus;

	private:
		void showLibrary();
		void openDocument(const std::string& path);
		void saveDocument(const std::string& text);
		void promptNewSkill();

		ui::FileSystem& mFiles;
		std::string mRoot;
		std::string mDocument;

		ui::PageStack mPages;
		ui::Widget mLibraryPage;
		ui::Widget mToolbar;
		ui::Button mNewSkill{ "New skill" };
		ui::Label mHint;
		ui::Widget mDocumentPage;

		ui::Dialog mNewSkillDialog;
		ui::TextField mNewSkillName;
		ui::Label mNewSkillError;
	};
}

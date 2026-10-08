#include "SecretsPanel.hpp"

#include "BoxLayout.hpp"
#include "Ui.hpp"

namespace tp::taskboard {

	using namespace ui;

	namespace {
		constexpr halnf kPadding = 12;
		constexpr halnf kGap = 6;
		constexpr halnf kKeyWidth = 200;
		constexpr halnf kValueWidth = 180;
		constexpr halnf kDescWidth = 240;

		bool isKey(const std::string& k) {
			if (k.empty()) return false;
			for (char ch : k)
				if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '.' && ch != '_' && ch != '-') return false;
			return true;
		}
	}

	SecretsPanel::SecretsPanel() {
		setDebugName("secrets panel");
		setSizing(AxisSizing::fill(), AxisSizing::fill());
		emplaceLayout<ColumnLayout>(kGap).setPadding(Insets::uniform(kPadding));

		mHint.setTone(Label::Tone::Muted);
		mHint.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mHint.setText("Agents read a value at run time with get_secret; the board never shows a stored value.");

		key.setPlaceholder("key");
		key.setSizing(AxisSizing::preferred(kKeyWidth), AxisSizing::fit());
		value.setPlaceholder("value");
		value.setSizing(AxisSizing::preferred(kValueWidth), AxisSizing::fit());
		description.setPlaceholder("description");
		description.setSizing(AxisSizing::fill(), AxisSizing::fit());
		save.setStyle(Button::Style::Accent);
		save.setSizing(AxisSizing::fit(), AxisSizing::fit());
		save.setOnClicked([this](const Event&) { commitForm(); });

		cancel.setStyle(Button::Style::Flat);
		cancel.setSizing(AxisSizing::fit(), AxisSizing::fit());
		cancel.setVisible(false);
		cancel.setOnClicked([this](const Event&) { clearForm(); });

		mForm.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mForm.emplaceLayout<RowLayout>(kGap);
		mForm.addChild(key);
		mForm.addChild(value);
		mForm.addChild(description);
		mForm.addChild(save);
		mForm.addChild(cancel);

		mStatus.setVisible(false);
		mStatus.setSizing(AxisSizing::fill(), AxisSizing::fit());

		mList.setSizing(AxisSizing::fill(), AxisSizing::fit());
		mList.emplaceLayout<ColumnLayout>(kGap);
		mListScroll.setContent(&mList);
		mListScroll.setSizing(AxisSizing::fill(), AxisSizing::fill());

		addChild(mHint);
		addChild(mForm);
		addChild(mStatus);
		addChild(mListScroll);
	}

	void SecretsPanel::setStore(SecretsStore* store) {
		mStore = store;
		refresh();
	}

	void SecretsPanel::refresh() {
		if (!mStore) { rebuildList({}); return; }
		mStore->listSecrets([this](std::vector<SecretEntry> entries) { rebuildList(entries); });
	}

	void SecretsPanel::rebuildList(const std::vector<SecretEntry>& entries) {
		mList.removeAllChildren();
		mRows.clear();

		for (const SecretEntry& entry : entries) {
			auto row = std::make_unique<Widget>();
			row->setSizing(AxisSizing::fill(), AxisSizing::fit());
			row->emplaceLayout<RowLayout>(kGap);

			auto keyLabel = std::make_unique<Label>(entry.key);
			keyLabel->setSizing(AxisSizing::preferred(kKeyWidth), AxisSizing::fit());

			auto descLabel = std::make_unique<Label>(entry.description);
			descLabel->setTone(Label::Tone::Muted);
			descLabel->setSizing(AxisSizing::fill(), AxisSizing::fit());

			auto editBtn = std::make_unique<Button>("Edit");
			editBtn->setStyle(Button::Style::Flat);
			editBtn->setSizing(AxisSizing::fit(), AxisSizing::fit());
			editBtn->setOnClicked([this, entry](const Event&) { beginEdit(entry); });

			auto delBtn = std::make_unique<Button>("Delete");
			delBtn->setStyle(Button::Style::Flat);
			delBtn->setSizing(AxisSizing::fit(), AxisSizing::fit());
			const std::string entryKey = entry.key;
			delBtn->setOnClicked([this, entryKey](const Event&) { removeSecret(entryKey); });

			row->addChild(*keyLabel);
			row->addChild(*descLabel);
			row->addChild(*editBtn);
			row->addChild(*delBtn);
			mList.addChild(*row);

			mRows.push_back(std::move(keyLabel));
			mRows.push_back(std::move(descLabel));
			mRows.push_back(std::move(editBtn));
			mRows.push_back(std::move(delBtn));
			mRows.push_back(std::move(row));
		}
		requestLayout();
	}

	void SecretsPanel::beginEdit(const SecretEntry& entry) {
		mEditing = true;
		key.setText(entry.key);
		value.setText("");
		description.setText(entry.description);
		value.setPlaceholder("new value");
		save.setText("Update");
		cancel.setVisible(true);
		mStatus.setVisible(false);
		if (ui()) ui()->setKeyboardFocus(&value);
	}

	void SecretsPanel::commitForm() {
		if (!mStore) { reportStatus("Server not connected.", true); return; }
		const std::string k = key.text();
		const std::string v = value.text();
		if (!isKey(k)) { reportStatus("Key must use letters, digits, '.', '_' or '-'.", true); return; }
		if (v.empty()) { reportStatus("Enter a value.", true); return; }
		mStore->setSecret(k, v, description.text(), [this, k](bool ok, std::string error) {
			if (!ok) { reportStatus("Could not store " + k + ": " + error, true); return; }
			reportStatus((mEditing ? "Updated " : "Stored ") + k + ".", false);
			clearForm();
			refresh();
		});
	}

	void SecretsPanel::clearForm() {
		mEditing = false;
		key.setText("");
		value.setText("");
		description.setText("");
		value.setPlaceholder("value");
		save.setText("Add");
		cancel.setVisible(false);
	}

	void SecretsPanel::removeSecret(const std::string& k) {
		if (!mStore) return;
		mStore->deleteSecret(k, [this, k](bool ok, std::string error) {
			if (!ok) { reportStatus("Could not delete " + k + ": " + error, true); return; }
			if (mEditing && key.text() == k) clearForm();
			reportStatus("Deleted " + k + ".", false);
			refresh();
		});
	}

	void SecretsPanel::reportStatus(const std::string& message, bool isError) {
		mStatus.setText(message);
		mStatus.setTone(isError ? Label::Tone::Warning : Label::Tone::Muted);
		mStatus.setVisible(true);
	}
}

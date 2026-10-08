#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Label.hpp"
#include "TaskStore.hpp"
#include "TextField.hpp"

#include <memory>
#include <string>
#include <vector>

namespace tp::taskboard {

	class SecretsPanel : public ui::Widget {
	public:
		SecretsPanel();

		void setStore(SecretsStore* store);
		void refresh();

		ui::TextField key;
		ui::TextField value;
		ui::TextField description;
		ui::Button save{ "Add" };
		ui::Button cancel{ "Cancel" };

	private:
		void rebuildList(const std::vector<SecretEntry>& entries);
		void beginEdit(const SecretEntry& entry);
		void commitForm();
		void removeSecret(const std::string& key);
		void clearForm();
		void reportStatus(const std::string& message, bool isError);

		SecretsStore* mStore = nullptr;
		bool mEditing = false;

		ui::Label mHint;
		ui::Label mStatus;
		ui::Widget mForm;

		ui::ScrollView mListScroll;
		ui::Widget mList;
		std::vector<std::unique_ptr<ui::Widget>> mRows;
	};
}

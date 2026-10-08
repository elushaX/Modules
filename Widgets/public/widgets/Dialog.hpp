#pragma once

#include "Button.hpp"
#include "Containers.hpp"
#include "Label.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	class Dialog : public Panel {
	public:
		explicit Dialog(std::string title = {});
		~Dialog() override;

		void setTitle(std::string title);
		void setMessage(std::string message);
		[[nodiscard]] const std::string& title() const { return mTitle.text(); }
		[[nodiscard]] const std::string& message() const { return mMessage.text(); }

		[[nodiscard]] Widget& body() { return mBody; }

		Button& addButton(std::string text, std::function<void()> action, bool isDefault = false);
		Button& addCancelButton(std::string text = "Cancel");
		[[nodiscard]] const std::vector<std::unique_ptr<Button>>& buttons() const { return mButtons; }
		void setOnCancel(std::function<void()> onCancel) { mOnCancel = std::move(onCancel); }

		void open(Ui& ui, Widget* owner = nullptr);
		void close();
		[[nodiscard]] bool isOpen() const;

		void accept();
		void cancel();

	protected:
		void onEvent(Event& event) override;

	private:
		Label mTitle;
		Label mMessage;
		Widget mBody;
		Widget mButtonRow;
		Spacer mButtonSpacer;
		std::vector<std::unique_ptr<Button>> mButtons;
		Button* mDefault = nullptr;
		std::function<void()> mOnCancel;
	};

	class ConfirmDialog : public Dialog {
	public:
		ConfirmDialog();

		void ask(Ui& ui, std::string title, std::string message, std::string confirmText, std::function<void()> onConfirm);

	private:
		std::function<void()> mOnConfirm;
		Button* mConfirm = nullptr;
	};
}

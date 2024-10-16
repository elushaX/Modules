#pragma once

#include "Widgets.hpp"

namespace tp {

	class UserWidget : public Widget {
	public:
		UserWidget() = default;

		void eventDraw(Canvas& canvas) override {
			if (this->isFocus()) canvas.rect(this->mArea, mAccentColor, mRounding);
			else canvas.rect(this->mArea, mBaseColor, mRounding);
			canvas.text(mUser.c_str(), this->mArea, mFontSize, Canvas::CC, mPadding, mUserColor);
		}

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("UserWidget");

			mBaseColor = wm.getColor("Base", "Base");
			mFontSize = wm.getNumber("Size", "FontSize");
			mPadding = wm.getNumber("Padding", "Padding");
			mUserColor = wm.getColor("ColUser", "Front");
			mAccentColor = wm.getColor("Accent", "Accent");
			mRounding = wm.getNumber("Rounding", "Rounding");
		}

	public:
		std::string mUser = "UserName";
		bool mIsHover = false;

		RGBA mBaseColor;
		RGBA mUserColor;
		RGBA mAccentColor;
		halnf mPadding = 0;
		halnf mFontSize = 0;
		halnf mRounding = 0;
	};

	class MessageWidget : public Widget {
	public:
		MessageWidget() = default;

		void eventDraw(Canvas& canvas) override {
			if (this->isFocus()) canvas.rect(this->mArea, mBaseColor, mRounding);

			auto userName = this->mArea;
			userName.w = 25;

			auto content = this->mArea;
			content.y = userName.y + userName.w;
			content.w = this->mArea.w - userName.w;

			canvas.text(mContent.c_str(), content, mFontSize, Canvas::LC, mPadding, mUserColorDim);
			canvas.text(mUser.c_str(), userName, mFontSizeDim, Canvas::LC, mPadding, mUserColor);
		}

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("MessageWidget");

			mBaseColor = wm.getColor("Base", "Base");
			mFontSize = wm.getNumber("Size", "FontSize");
			mFontSizeDim = wm.getNumber("SizeUser", "FontSizeDim");
			mPadding = wm.getNumber("Padding", "Padding");
			mUserColor = wm.getColor("UserColor", "Front");
			mUserColorDim = wm.getColor("UserColorDim", "FrontDim");
			mRounding = wm.getNumber("Rounding", "Rounding");
		}

	public:
		std::string mContent = "Message Content";
		std::string mUser = "UserName";
		bool mIsHover = false;

		RGBA mBaseColor;
		RGBA mUserColor;
		RGBA mUserColorDim;
		halnf mPadding = 0;
		halnf mFontSize = 0;
		halnf mFontSizeDim = 0;
		halnf mRounding = 0;
	};

	class LoginWidget : public Widget {
	public:
		explicit LoginWidget() {
			mPass.mId = "pass";
			mUser.mId = "user";
			mButton.mLabel.mLabel = "Login";

			this->mChildWidgets.pushBack(&mPass);
			this->mChildWidgets.pushBack(&mUser);
			this->mChildWidgets.pushBack(&mButton);
		}

		void eventProcess(const Events& events) override {
			mLogged = false;

			const auto xval = this->mArea.z / 2 - 100;

			mUser.setArea({ xval, 10, 200, 30 });
			mPass.setArea({ xval, 50, 200, 30 });
			mButton.setArea({ xval, 90, 200, 30 });

			if (mButton.isFired()) {
				mLogged = true;
			}
		}

		void eventDraw(Canvas& canvas) override { canvas.rect(this->mArea, mBGColor); }

	public:
		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("LoginWidget");
			mBGColor = wm.getColor("Back", "Base");
		}

	public:
		TextInputWidget mUser;
		TextInputWidget mPass;
		ButtonWidget mButton;
		bool mLogged = false;

		RGBA mBGColor;
	};

	class ActiveChatWidget : public Widget {
	public:
		ActiveChatWidget() {
			mSend.mLabel.mLabel = "Send";
			mMessage.mId = "Message";

			this->mChildWidgets.pushBack(&mHistoryView);
			this->mChildWidgets.pushBack(&mMessage);
			this->mChildWidgets.pushBack(&mSend);
		}

		void eventProcess(const Events& events) override {
			auto history = this->mArea;
			history.w -= 50;

			auto input = this->mArea;
			input.y = history.w + 10;
			input.w = 40 - mPadding;
			input.x += mPadding;
			input.z -= mPadding;

			auto inputMessage = input;
			inputMessage.z -= 100;

			auto inputSend = input;
			inputSend.x = inputMessage.x + inputMessage.z + mPadding;
			inputSend.z = 100 - mPadding * 2;

			mSend.setArea(inputSend);
			mMessage.setArea(inputMessage);

			mHistoryView.setArea(history);
		}

		void eventDraw(Canvas& canvas) override { canvas.rect(this->mArea, mBGColor); }

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("ActiveChat");
			mBGColor = wm.getColor("Back", "Background");
			mPadding = wm.getNumber("Padding", "Padding");
		}

	public:
		Buffer<MessageWidget> mMessages;
		ScrollableWindow mHistoryView;
		TextInputWidget mMessage;
		ButtonWidget mSend;

		RGBA mBGColor;
		halnf mPadding = 0;
	};

	class ChattingWidget : public DockWidget {
	public:
		ChattingWidget() {
			// todo :  fetch code
			mUsers.append(UserWidget());
			mUsers.append(UserWidget());
			mUsers.append(UserWidget());

			mUsers[0].mArea = { 0, 0, 100, 100 };
			mUsers[1].mArea = { 0, 0, 100, 100 };
			mUsers[2].mArea = { 0, 0, 100, 100 };

			for (auto message : mUsers) {
				mSideView.addWidget(&message.data());
			}

			mActive.mMessages.append(MessageWidget());
			mActive.mMessages.append(MessageWidget());
			mActive.mMessages.append(MessageWidget());

			mActive.mMessages[0].mArea = { 0, 0, 100, 50 };
			mActive.mMessages[1].mArea = { 0, 0, 100, 50 };
			mActive.mMessages[2].mArea = { 0, 0, 100, 50 };

			for (auto message : mActive.mMessages) {
				mActive.mHistoryView.addWidget(&message.data());
			}

			addSideWidget(&mSideView, DockWidget::RIGHT);
			setCenterWidget(&mActive);
		}

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("ChattingWidget");
			mBGColor = wm.getColor("Back", "Background");
		}

	public:
		Buffer<UserWidget> mUsers;
		ScrollableWindow mSideView;
		ActiveChatWidget mActive;

		RGBA mBGColor;
	};

	class ComplexWidget : public Widget {
	public:
		ComplexWidget() {
			this->mChildWidgets.pushBack(&mLogin);
			this->mChildWidgets.pushBack(&mChatting);
		}

		void eventProcess(const Events& events) override {
			mLogged = mLogin.mLogged;

			mLogin.setEnable(!mLogged);
			mChatting.setEnable(mLogged);

			mLogin.setArea(this->mArea);
			mChatting.setArea(this->mArea);
		}

		void eventDraw(Canvas& canvas) override { canvas.rect(this->mArea, mBGColor); }

		void eventUpdateConfiguration(WidgetManager& wm) override {
			wm.setActiveId("ChatGui");
			mBGColor = wm.getColor("Back", "Background");
		}

	private:
		bool mLogged = false;
		LoginWidget mLogin;
		ChattingWidget mChatting;

		RGBA mBGColor;
	};
}
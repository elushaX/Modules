#pragma once

#include "Widgets.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class UserWidget : public Widget<Events, Canvas> {
	public:
		UserWidget() = default;

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			this->mArea.w = 30;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;
			mIsHover = aArea.isInside(events.getPointer());
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

			if (mIsHover) canvas.rect(this->mArea, mAccentColor, mRounding);
			else canvas.rect(this->mArea, mBaseColor, mRounding);

			canvas.text(mUser.c_str(), this->mArea, mFontSize, Canvas::CC, mPadding, mUserColor);
		}

	public:
		void updateConfigCache(WidgetManager& wm) override {
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

	template <typename Events, typename Canvas>
	class MessageWidget : public Widget<Events, Canvas> {
	public:
		MessageWidget() = default;

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			this->mArea.w = 50;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;
			mIsHover = aArea.isInside(events.getPointer());
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;
			if (mIsHover) canvas.rect(this->mArea, mBaseColor, mRounding);

			auto userName = this->mArea;
			userName.w = 25;

			auto content = this->mArea;
			content.y = userName.y + userName.w;
			content.w = this->mArea.w - userName.w;

			canvas.text(mContent.c_str(), content, mFontSize, Canvas::LC, mPadding, mUserColorDim);
			canvas.text(mUser.c_str(), userName, mFontSizeDim, Canvas::LC, mPadding, mUserColor);
		}

		void updateConfigCache(WidgetManager& wm) override {
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

	template <typename Events, typename Canvas>
	class LoginWidget : public Widget<Events, Canvas> {
	public:
		explicit LoginWidget() {
			mPass.mId = "pass";
			mUser.mId = "user";
			mButton.mLabel.mLabel = "Login";
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			mLogged = false;

			const auto xval = aArea.z / 2 - 100;
			mUser.proc(events, aArea, { xval, 10, 200, 30 });
			mPass.proc(events, aArea, { xval, 50, 200, 30 });
			mButton.proc(events, aArea, { xval, 90, 200, 30 });

			if (mButton.mIsReleased) {
				mButton.mIsReleased = false;
				mLogged = true;
			}
		}

		void draw(Canvas& canvas) override {
			canvas.rect(this->mArea, mBGColor);
			mButton.draw(canvas);
			mUser.draw(canvas);
			mPass.draw(canvas);
		}

	public:
		void updateConfigCache(WidgetManager& wm) override {
			wm.setActiveId("LoginWidget");

			mBGColor = wm.getColor("Back", "Base");

			mUser.updateConfigCache(wm);
			mPass.updateConfigCache(wm);
			mButton.updateConfigCache(wm);
		}

	public:
		TextInputWidget<Events, Canvas> mUser;
		TextInputWidget<Events, Canvas> mPass;
		ButtonWidget<Events, Canvas> mButton;
		bool mLogged = false;

		RGBA mBGColor;
	};

	template <typename Events, typename Canvas>
	class ActiveChatWidget : public Widget<Events, Canvas> {
	public:
		ActiveChatWidget() {
			mSend.mLabel.mLabel = "Send";
			mMessage.mId = "Message";
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;

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

			mSend.proc(events, this->mArea, inputSend);
			mMessage.proc(events, this->mArea, inputMessage);

			if (mSend.mIsReleased) {
				mSend.mIsReleased = false;
			}

			mHistoryView.proc(events, this->mArea, history);
		}

		void draw(Canvas& canvas) override {
			canvas.rect(this->mArea, mBGColor);
			mHistoryView.draw(canvas);
			mMessage.draw(canvas);
			mSend.draw(canvas);
		}

		void updateConfigCache(WidgetManager& wm) override {
			wm.setActiveId("ActiveChat");

			mBGColor = wm.getColor("Back", "Background");
			mPadding = wm.getNumber("Padding", "Padding");

			mHistoryView.updateConfigCache(wm);
			mMessage.updateConfigCache(wm);
			mSend.updateConfigCache(wm);

			for (auto message : mMessages) {
				message->updateConfigCache(wm);
			}
		}

	public:
		Buffer<MessageWidget<Events, Canvas>> mMessages;
		ScrollableWindow<Events, Canvas> mHistoryView;
		TextInputWidget<Events, Canvas> mMessage;
		ButtonWidget<Events, Canvas> mSend;

		RGBA mBGColor;
		halnf mPadding = 0;
	};

	template <typename Events, typename Canvas>
	class ChattingWidget : public Widget<Events, Canvas> {
	public:
		ChattingWidget() {
			// todo :  fetch code
			mUsers.append(UserWidget<Events, Canvas>());
			mUsers.append(UserWidget<Events, Canvas>());
			mUsers.append(UserWidget<Events, Canvas>());

			mUsers[0].mArea = { 0, 0, 100, 100 };
			mUsers[1].mArea = { 0, 0, 100, 100 };
			mUsers[2].mArea = { 0, 0, 100, 100 };

			for (auto message : mUsers) {
				mSideView.mContents.append(&message.data());
			}

			mActive.mMessages.append(MessageWidget<Events, Canvas>());
			mActive.mMessages.append(MessageWidget<Events, Canvas>());
			mActive.mMessages.append(MessageWidget<Events, Canvas>());

			mActive.mMessages[0].mArea = { 0, 0, 100, 100 };
			mActive.mMessages[1].mArea = { 0, 0, 100, 100 };
			mActive.mMessages[2].mArea = { 0, 0, 100, 100 };

			for (auto message : mActive.mMessages) {
				mActive.mHistoryView.mContents.append(&message.data());
			}
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;

			mSplitView.proc(events, aArea, aArea);
			mSideView.proc(events, this->mArea, mSplitView.getSecond());
			mActive.proc(events, aArea, mSplitView.getFirst());
		}

		void draw(Canvas& canvas) override {
			canvas.rect(this->mArea, mBGColor);
			mSplitView.draw(canvas);
			mSideView.draw(canvas);
			mActive.draw(canvas);
		}

		void updateConfigCache(WidgetManager& wm) override {
			wm.setActiveId("ChattingWidget");

			mBGColor = wm.getColor("Back", "Background");

			mSideView.updateConfigCache(wm);
			mActive.updateConfigCache(wm);
			mSplitView.updateConfigCache(wm);

			for (auto user : mUsers) {
				user->updateConfigCache(wm);
			}
		}

	public:
		Buffer<UserWidget<Events, Canvas>> mUsers;
		ScrollableWindow<Events, Canvas> mSideView;
		ActiveChatWidget<Events, Canvas> mActive;
		SplitView<Events, Canvas> mSplitView;

		RGBA mBGColor;
	};

	template <typename Events, typename Canvas>
	class ComplexWidget : public Widget<Events, Canvas> {
	public:
		ComplexWidget() = default;

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			if (mLogged) {
				mChatting.proc(events, aArea, aArea);
			} else {
				mLogin.proc(events, aArea, aArea);
				mLogged = mLogin.mLogged;
				if (mLogged) {
					mChatting.proc(events, aArea, aArea);
				}
			}
		}

		void draw(Canvas& canvas) override {
			canvas.rect(this->mArea, mBGColor);
			if (mLogged) mChatting.draw(canvas);
			else mLogin.draw(canvas);
		}

		void updateConfigCache(WidgetManager& wm) override {
			wm.setActiveId("ChatGui");

			mBGColor = wm.getColor("Back", "Background");

			mLogin.updateConfigCache(wm);
			mChatting.updateConfigCache(wm);
		}

	private:
		bool mLogged = false;
		LoginWidget<Events, Canvas> mLogin;
		ChattingWidget<Events, Canvas> mChatting;

		RGBA mBGColor;
	};
}
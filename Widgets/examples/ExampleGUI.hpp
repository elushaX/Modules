#pragma once

#include "Widgets.hpp"

namespace tp {

	template <typename Events, typename Canvas>
	class UserWidget : public Widget<Events, Canvas> {
	public:
		UserWidget() {
			this->createConfig("User");
			this->addColor("Base", "Base");
			this->addValue("Size", "FontSize");
			this->addValue("Padding", "Padding");
			this->addColor("ColUser", "Front");
			this->addColor("Accent", "Accent");
			this->addValue("Rounding", "Rounding");
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			this->mArea.w = 30;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;
			mIsHover = aArea.isInside(events.getPos());
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

			if (mIsHover) canvas.rect(this->mArea, this->getColor("Accent"), this->getValue("Rounding"));
			else canvas.rect(this->mArea, this->getColor("Base"), this->getValue("Rounding"));

			const auto padding = this->getValue("Padding");
			const auto size = this->getValue("Size");
			const auto colUser = this->getColor("ColUser");

			canvas.text(mUser.read(), this->mArea, size, Canvas::CC, padding, colUser);
		}

	public:
		String mUser = "UserName";
		bool mIsHover = false;
	};

	template <typename Events, typename Canvas>
	class MessageWidget : public Widget<Events, Canvas> {
	public:
		MessageWidget() {
			this->createConfig("Message");
			this->addColor("Base", "Base");
			this->addValue("Size", "FontSize");
			this->addValue("SizeUser", "FontSizeDim");
			this->addValue("Padding", "Padding");
			this->addColor("ColUser", "Front");
			this->addColor("Col", "FrontDim");
			this->addValue("Rounding", "Rounding");
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;
			this->mArea.w = 50;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;
			mIsHover = aArea.isInside(events.getPos());
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

			if (mIsHover) canvas.rect(this->mArea, this->getColor("Base"), this->getValue("Rounding"));

			const auto padding = this->getValue("Padding");
			const auto size = this->getValue("Size");
			const auto sizeUser = this->getValue("SizeUser");
			const auto col = this->getColor("Col");
			const auto colUser = this->getColor("ColUser");

			auto userName = this->mArea;
			userName.w = 25;

			auto content = this->mArea;
			content.y = userName.y + userName.w;
			content.w = this->mArea.w - userName.w;

			canvas.text(mContent.read(), content, size, Canvas::LC, padding, col);
			canvas.text(mUser.read(), userName, sizeUser, Canvas::LC, padding, colUser);
		}

	public:
		String mContent = "Message Content";
		String mUser = "UserName";
		bool mIsHover = false;
	};

	template <typename Events, typename Canvas>
	class LoginWidget : public Widget<Events, Canvas> {
	public:
		explicit LoginWidget() {
			this->createConfig("Login");
			this->addColor("Back", "Background");
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
			canvas.rect(this->mArea, this->getColor("Back"));
			mButton.draw(canvas);
			mUser.draw(canvas);
			mPass.draw(canvas);
		}

	public:
		TextInputWidget<Events, Canvas> mUser;
		TextInputWidget<Events, Canvas> mPass;
		ButtonWidget<Events, Canvas> mButton;
		bool mLogged = false;
	};

	template <typename Events, typename Canvas>
	class ActiveChatWidget : public Widget<Events, Canvas> {
	public:
		ActiveChatWidget() {
			this->createConfig("ActiveChat");
			this->addColor("Back", "Background");
			this->addValue("Padding", "Padding");
			mSend.mLabel.mLabel = "Send";
			mMessage.mId = "Message";
		}

		void proc(const Events& events, const RectF& areaParent, const RectF& aArea) override {
			this->mArea = aArea;

			auto history = this->mArea;
			history.w -= 50;

			auto input = this->mArea;
			input.y = history.w + 10;
			input.w = 40 - this->getValue("Padding");
			input.x += this->getValue("Padding");
			input.z -= this->getValue("Padding");

			auto inputMessage = input;
			inputMessage.z -= 100;

			auto inputSend = input;
			inputSend.x = inputMessage.x + inputMessage.z + this->getValue("Padding");
			inputSend.z = 100 - this->getValue("Padding") * 2;

			mSend.proc(events, this->mArea, inputSend);
			mMessage.proc(events, this->mArea, inputMessage);

			if (mSend.mIsReleased) {
				mSend.mIsReleased = false;
			}

			mHistoryView.proc(events, this->mArea, history);
		}

		void draw(Canvas& canvas) override {
			canvas.rect(this->mArea, this->getColor("Back"));
			mHistoryView.draw(canvas);
			mMessage.draw(canvas);
			mSend.draw(canvas);
		}

	public:
		Buffer<MessageWidget<Events, Canvas>> mMessages;
		ScrollableWindow<Events, Canvas> mHistoryView;
		TextInputWidget<Events, Canvas> mMessage;
		ButtonWidget<Events, Canvas> mSend;
	};

	template <typename Events, typename Canvas>
	class ChattingWidget : public Widget<Events, Canvas> {
	public:
		explicit ChattingWidget() {
			this->createConfig("Chatting");
			this->addColor("Back", "Background");
			this->addValue("Padding", "Padding");

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

			const auto padding = this->getValue("Padding");
			mSideView.proc(events, this->mArea, mSplitView.getSecond());

			mActive.proc(events, aArea, mSplitView.getFirst());
		}

		void draw(Canvas& canvas) override {
			canvas.rect(this->mArea, this->getColor("Back"));
			mSplitView.draw(canvas);
			mSideView.draw(canvas);
			mActive.draw(canvas);
		}

	public:
		Buffer<UserWidget<Events, Canvas>> mUsers;
		ScrollableWindow<Events, Canvas> mSideView;
		ActiveChatWidget<Events, Canvas> mActive;
		SplitView<Events, Canvas> mSplitView;
	};

	template <typename Events, typename Canvas>
	class ComplexWidget : public Widget<Events, Canvas> {
	public:
		ComplexWidget() {
			this->createConfig("chat");
			this->addColor("Back", "Background");
		}

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
			canvas.rect(this->mArea, this->getColor("Back"));
			if (mLogged) mChatting.draw(canvas);
			else mLogin.draw(canvas);
		}

	private:
		bool mLogged = false;
		LoginWidget<Events, Canvas> mLogin;
		ChattingWidget<Events, Canvas> mChatting;
	};
}
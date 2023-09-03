
#include "NewPlacement.hpp"

#include "Chat.hpp"

#include "Graphics.hpp"

using namespace tp;

static ModuleManifest* deps[] = { &gModuleGraphics, nullptr };

ModuleManifest tp::gModuleChat = ModuleManifest("Chat", nullptr, nullptr, deps );

bool ChatAPI::isLogged() const {
	return mLogged;
}

void ChatAPI::login(const Credentials& credentials) {
	mLogged = String::Logic::isEqualLogic(credentials.pass, "1812") && String::Logic::isEqualLogic(credentials.name, "Igor");
	mChatsList = { "Serg", "Friends" };
	mChatHistories.put("Serg", { { "Serg", "Hello wanna some coke-cola?" }, { "Serg", "Will be in 5 mins" } });
	mChatHistories.put("Friends", { { "Artem", "Whos is that Serg?" }, { "Erog", "idunno" } });
}

void ChatAPI::logout() {
	mLogged = false;
	mChatsList.removeAll();
	mChatHistories.removeAll();
}

const ChatAPI::ChatsList* ChatAPI::getChatsList() const {
	return mLogged ? &mChatsList : nullptr;
}

const ChatAPI::ChatHistory* ChatAPI::getChatHistory(const String& chatName) const {
	if (!mLogged) return nullptr;

	auto history = mChatHistories.presents(chatName);
	return history ? &mChatHistories.getSlotVal(history) : nullptr;
}

void ChatAPI::send(const String& chatName, const Message& message) {
	if (!mLogged) return;

	auto iter = mChatHistories.presents(chatName);
	auto history = iter ? &mChatHistories.getSlotVal(iter) : nullptr;

	if (!history) return;

	history->pushBack(message);
}

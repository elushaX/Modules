
#include "Chat.hpp"

#include "Graphics.hpp"

using namespace tp;

static ModuleManifest* deps[] = { &gModuleGraphics, nullptr };

ModuleManifest tp::gModuleChat = ModuleManifest("Chat", nullptr, nullptr, deps );

bool ChatAPI::isLogged() const {
	return mLogged;
}

void ChatAPI::login(const Credentials& credentials) {
	mLogged = String::Logic::isEqualLogic(credentials.name, "111");
}
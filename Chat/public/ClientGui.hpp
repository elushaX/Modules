#pragma once

#include "Chat.hpp"

namespace tp {
	class ClientGUI {
		enum { MESSAGE_SIZE = 1024 };
		ChatAPI::Credentials mCredentials;
		bool mInvalidCredentials = false;
		char mMessageBuff[1000] {0};
		String mActiveChatName = "Not Selected";

	public:
		ClientGUI() = default;
		void draw(ChatAPI& api);

	private:
		void drawChat(ChatAPI& api);
	};
}
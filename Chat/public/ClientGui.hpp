#pragma once

#include "Chat.hpp"

namespace tp {
	class ClientGUI {
		ChatAPI::Credentials mCredentials;
		bool mInvalidCredentials = false;
	public:
		ClientGUI() = default;
		void draw(ChatAPI& api);
	};
}
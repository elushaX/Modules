#pragma once

#include "Module.hpp"

namespace tp {

	extern ModuleManifest gModuleChat;

	class ChatAPI {
	public:
		struct Credentials {
			enum { SIZE = 64 };
			tp::int1 name[SIZE] {0};
			tp::int1 pass[SIZE] {0};
		};

	public:
		ChatAPI() = default;
		[[nodiscard]] bool isLogged() const;
		void login(const Credentials& credentials);

	private:
		bool mLogged = false;
		Credentials mCredentials;
	};
}


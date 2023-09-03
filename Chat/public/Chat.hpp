#pragma once

#include "Strings.hpp"
#include "Map.hpp"
#include "List.hpp"

namespace tp {

	extern ModuleManifest gModuleChat;

	class ChatAPI {
	public:
		struct Credentials {
			enum { SIZE = 64 };
			tp::int1 name[SIZE] {0};
			tp::int1 pass[SIZE] {0};
		};

		typedef List<String> ChatsList;

		struct Message {
			String user;
			String text;
		};

		typedef List<Message> ChatHistory;

	public:
		ChatAPI() = default;
		[[nodiscard]] bool isLogged() const;
		void login(const Credentials& credentials);
		[[nodiscard]] const ChatsList* getChatsList() const;
		[[nodiscard]] const ChatHistory* getChatHistory(const String& chatName) const;
		void logout();
		void send(const String& chatName, const Message& message);

	private:
		bool mLogged = false;
		Credentials mCredentials;
		ChatsList mChatsList;

		Map<String, ChatHistory> mChatHistories;
	};
}


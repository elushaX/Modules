
#include "SystemAPI.hpp"
#include "Strings.hpp"
#include "List.hpp"

#include <pthread.h>

using namespace tp;

class ChatServer {

	struct SharedData {
		List<Server::Socket> clients;
		pthread_mutex_t mutex;
	};

	SharedData mSharedData;
	Server server;

public:
	explicit ChatServer(int port) {
		server.start(port);
	}

	~ChatServer() {
		DEBUG_ASSERT(false)
	}

	bool start() {
		while (true) {
			auto clientSocket = server.accept();

			// add client
			pthread_mutex_lock(&mSharedData.mutex);
			mSharedData.clients.pushBack(clientSocket);
			pthread_mutex_unlock(&mSharedData.mutex);

			// Create a new thread to handle the client
			pthread_t threadId;
			if (pthread_create(&threadId, nullptr, (void* (*)(void*)) handleClient, this)) {
				printf("Failed to create thread for client\n");
				return false;
			}

			// Detach the thread so it can run independently
			pthread_detach(threadId);
		}
		return true;
	}


	static void* handleClient(ChatServer* self) {
		// read shared data - current client id
		pthread_mutex_lock(&self->mSharedData.mutex);
		auto clientSocket = self->mSharedData.clients.last();
		pthread_mutex_unlock(&self->mSharedData.mutex);

	MESSAGE:
		// wait for a message request
		auto message = Server::read(clientSocket);

		// Broadcast to all clients
		for (auto client : self->mSharedData.clients) {
			Server::write(client.data(), message);
		}

		auto exit = memCompare(message, "exit", 5) == 0;

		delete[] message;

		if (exit) {
			// TODO : disconnect
			for (auto iter : self->mSharedData.clients) {
				if (clientSocket == iter.data()) {
					self->mSharedData.clients.removeNode(iter.node());
					break;
				}
			}
		}

		pthread_mutex_unlock(&self->mSharedData.mutex);

		if (exit) return nullptr;
		goto MESSAGE;
	}
};

int main() {
	ChatServer server(5555);
	server.start();
	return 0;
}

#include "SystemAPI.hpp"

#include "Strings.hpp"

#include <pthread.h>

using namespace tp;

class CharClient {

	typedef void* (*ThreadFunction)(void *);

	Client client;
	pthread_mutex_t mutex;

public:
	CharClient(int port, const char* ip) {
		pthread_mutex_unlock(&mutex);
		client.connect(ip, port);
	}

	void start() {
		// Create a new thread to handle the client
		pthread_t threadId;
		if (pthread_create(&threadId, nullptr, (ThreadFunction) readServerBroadcast, this)) {
			printf("Failed to create thread for client\n");
			return;
		}

		// Detach the thread so it can run independently
		pthread_detach(threadId);

		while (true) {
			const auto length =  1024;
			static char message[length];

			printf(" >> ");
			fgets(message, length, stdin);

			if (String::Logic::isEqualLogic(message, "exit")) return;

			pthread_mutex_lock(&mutex);
			client.write(message);
			pthread_mutex_unlock(&mutex);
		}
	}

	static void* readServerBroadcast(CharClient* self) {
		while (true) {
			auto message = self->client.read();
			pthread_mutex_lock(&self->mutex);
			printf("Broadcast : %s \n", message);
			delete[] message;
			pthread_mutex_unlock(&self->mutex);
		}
		return nullptr;
	}
};

int main() {
	CharClient client(5555, "127.0.0.1");
	client.start();
	return 0;
}

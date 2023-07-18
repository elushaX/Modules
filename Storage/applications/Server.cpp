#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>


class Server {
public:
	Server(int port) : serverSocket(-1), port(port) {}

	bool start() {
		// Create socket
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (serverSocket == -1) {
			std::cerr << "Failed to create socket" << std::endl;
			return false;
		}

		// Bind socket to port
		sockaddr_in serverAddress{};
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(port);
		serverAddress.sin_addr.s_addr = INADDR_ANY;
		if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
			std::cerr << "Failed to bind socket" << std::endl;
			return false;
		}

		// Listen for connections
		if (listen(serverSocket, 5) == -1) {
			std::cerr << "Failed to listen on socket" << std::endl;
			return false;
		}

		std::cout << "Server listening on port " << port << std::endl;

		// Start accepting clients
		while (true) {
			// Accept client connection
			sockaddr_in clientAddress{};
			socklen_t clientAddressSize = sizeof(clientAddress);
			int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
			if (clientSocket == -1) {
				std::cerr << "Failed to accept client connection" << std::endl;
				return false;
			}

			// Create a new thread to handle the client
			pthread_t threadId;
			if (pthread_create(&threadId, nullptr, handleClient, &clientSocket) != 0) {
				std::cerr << "Failed to create thread for client" << std::endl;
				return false;
			}

			// Detach the thread so it can run independently
			pthread_detach(threadId);
		}

		// Close server socket
		close(serverSocket);

		return true;
	}

private:
	int serverSocket;
	int port;

	static void* handleClient(void* clientSocketPtr) {
		int clientSocket = *(reinterpret_cast<int*>(clientSocketPtr));
		constexpr int BUFFER_SIZE = 1024;

		// Receive and print client message
		char buffer[BUFFER_SIZE];
		ssize_t bytesRead = read(clientSocket, buffer, BUFFER_SIZE - 1);
		if (bytesRead == -1) {
			std::cerr << "Failed to read from socket" << std::endl;
			return nullptr;
		}

		buffer[bytesRead] = '\0';
		std::cout << "Received message from client: " << buffer << std::endl;

		// Close socket
		close(clientSocket);

		return nullptr;
	}
};

int main() {
	Server server(8080);
	server.start();

	return 0;
}

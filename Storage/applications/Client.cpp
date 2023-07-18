#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
const char* SERVER_IP = "127.0.0.1";

int main() {
	// Create socket
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == -1) {
		std::cerr << "Failed to create socket" << std::endl;
		return 1;
	}

	// Connect to server
	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	if (inet_pton(AF_INET, SERVER_IP, &serverAddress.sin_addr) <= 0) {
		std::cerr << "Invalid address or address not supported" << std::endl;
		return 1;
	}

	if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
		std::cerr << "Failed to connect to server" << std::endl;
		return 1;
	}

	// Send message to server
	const char* message = "Hello, server!";
	if (write(clientSocket, message, strlen(message)) == -1) {
		std::cerr << "Failed to write to socket" << std::endl;
		return 1;
	}

	// Close socket
	close(clientSocket);

	return 0;
}

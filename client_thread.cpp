#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>

// Function to continuously receive messages from server
void receive_messages(int sock) {
    char buffer[1024];
    while (true) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cout << "Disconnected from server.\n";
            close(sock);
            break;
        }
        buffer[bytes_received] = '\0';
        std::cout << "\nServer: " << buffer << std::endl;
        std::cout << "You: " << std::flush; // reprint prompt
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed.\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // change to server IP if needed

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed.\n";
        return 1;
    }

    std::cout << "✅ Connected to server. Type 'bye' to exit.\n";

    // Start a separate thread to receive messages
    std::thread receiver(receive_messages, sock);
    receiver.detach();

    // Main thread handles sending
    while (true) {
        std::string msg;
        std::cout << "You: ";
        std::getline(std::cin, msg);

        send(sock, msg.c_str(), msg.size(), 0);

        if (msg == "bye") {
            break;
        }
    }

    close(sock);
    return 0;
}

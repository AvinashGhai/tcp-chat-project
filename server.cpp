#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <map>
#include <algorithm>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 1024

std::mutex clients_mutex;
std::map<int, std::string> client_names; // socket -> name
std::map<std::string, int> name_to_socket; // name -> socket

void send_to_client(int client_socket, const std::string &message) {
    send(client_socket, message.c_str(), message.size(), 0);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];

    // First message from client is the name
    memset(buffer, 0, BUFFER_SIZE);
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        close(client_socket);
        return;
    }

    std::string name(buffer);

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        client_names[client_socket] = name;
        name_to_socket[name] = client_socket;
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::cout << name << " connected!" << std::endl;
    }

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) break;

        std::string msg = name + ": " + buffer;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            std::cout << msg << std::endl;
        }
    }

    // Client disconnects
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::cout << name << " disconnected." << std::endl;
        client_names.erase(client_socket);
        name_to_socket.erase(name);
    }

    close(client_socket);
}

// Separate thread for server input to reply to clients
void server_input() {
    while (true) {
        std::string input;
        std::getline(std::cin, input);

        // Format: ClientName: message
        size_t colon = input.find(':');
        if (colon == std::string::npos) {
            std::cout << "Use format: ClientName: message" << std::endl;
            continue;
        }

        std::string target_name = input.substr(0, colon);
        std::string message = input.substr(colon + 1);
        message = "Server: " + message;

        std::lock_guard<std::mutex> lock(clients_mutex);
        if (name_to_socket.find(target_name) != name_to_socket.end()) {
            int sock = name_to_socket[target_name];
            send_to_client(sock, message);
        } else {
            std::cout << "Client not found: " << target_name << std::endl;
        }
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Start server input thread
    std::thread(server_input).detach();

    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        std::thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return 0;
}
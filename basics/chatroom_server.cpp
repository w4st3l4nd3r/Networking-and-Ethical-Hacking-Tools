// === TCP CHATROOM SERVER ===
// This simple TCP chatroom server listens for client connections on port 8080.
// It then receives client messages which it broadcasts back to all connected clients.
// Server Commands: /kick <fd>, /list
// Client Commands: /exit

// Currently hardcoded for "127.0.0.1" localhost connections.

#include <algorithm>
#include <arpa/inet.h>
#include <iostream>
#include <map>
#include <mutex>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <vector>

class ServerSocket {
    private:
    int serverSocketFileDescriptor;
    struct sockaddr_in serverSocketAddress;
    socklen_t serverSize = sizeof(serverSocketAddress);   

    public:
    ServerSocket(){};
    ~ServerSocket(){};

    int getSSFD() {
        return serverSocketFileDescriptor;
    }

    bool initialize() {
        serverSocketFileDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocketFileDescriptor == -1) {
            std::cerr << "[!] socket() failed to initialize: " << strerror(errno) << std::endl;
            return false;
        }

        int opt = 1;
        setsockopt(serverSocketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        serverSocketAddress.sin_family = AF_INET;                  
        serverSocketAddress.sin_addr.s_addr = INADDR_ANY;          
        serverSocketAddress.sin_port = htons(8080);                

        if (bind(serverSocketFileDescriptor, (struct sockaddr*) &serverSocketAddress, serverSize) == -1) {
            std::cerr << "[!] bind() failed to initialize: " << strerror(errno) << std::endl;
            closeServerSocket();
            return false;
        }

        return true;
    }

    void closeServerSocket() {
        std::cout << "Shutting down server socket..." << std::endl;
        close(serverSocketFileDescriptor);
        return;
    }
};

class ClientSocket {
    private:
    int clientSocketFileDescriptor;    

    std::vector<int> activeClientsSFD;
    std::mutex clientMutex;

    void handleClient(int clientSFD) {
        char buffer[1024];

        while (true) {
            memset(buffer, 0, sizeof(buffer));

            int bytesReceived = recv(clientSFD, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                std::lock_guard<std::mutex> lock(clientMutex);
                std::cout << "[*] Client disconnected: FD " << clientSFD << std::endl;
                close(clientSFD);
                activeClientsSFD.erase(std::remove(activeClientsSFD.begin(), activeClientsSFD.end(), clientSFD), activeClientsSFD.end());
                return;
            }

            std::string message = "[FD " + std::to_string(clientSFD) + "] " + buffer;

            // Broadcast to all other clients:
            for (int client : activeClientsSFD) {
                if (client != clientSFD) {
                    send(client, message.c_str(), message.length(), 0);
                }
            }
        }
    }

    void adminCommandLoop() {
        std::string command;
        while (true) {
            std::getline(std::cin, command);

            if (command == "/list") {
                std::lock_guard<std::mutex> lock(clientMutex);
                std::cout << "[*] Active clients:" << std::endl;
                for (int fd : activeClientsSFD) {
                    std::cout << fd << std::endl;
                }
            } else if (command.substr(0, 6) == "/kick ") {
                int targetFD = std::stoi(command.substr(6));
                std::lock_guard<std::mutex> lock(clientMutex);
                auto it = std::find(activeClientsSFD.begin(), activeClientsSFD.end(), targetFD);
                if (it != activeClientsSFD.end()) {
                    std::cout << "[!] Kicking client FD " << targetFD << std::endl;
                    std::string kickNotice = "[SERVER] You have been kicked.\n";
                    send(targetFD, kickNotice.c_str(), kickNotice.size(), 0);
                    shutdown(targetFD, SHUT_RDWR);
                    close(targetFD);
                    activeClientsSFD.erase(it);
                } else {
                    std::cout << "[!] No such client FD: " << targetFD << std::endl;
                }
            }
            else {
                std::cout << "[!] No such command." << std::endl;
            }
        }
    }

    public:
    ClientSocket(){};
    ~ClientSocket() {
        closeClientSocket();
    }

    void initialize(int SSFD) {       
        while (true) {    
            clientSocketFileDescriptor = 0;
            struct sockaddr_in clientSocketAddress;
            socklen_t clientSize = sizeof(clientSocketAddress);

            std::thread adminThread(&ClientSocket::adminCommandLoop, this);
            adminThread.detach();

            clientSocketFileDescriptor = accept(SSFD, (struct sockaddr*) &clientSocketAddress, &clientSize);
            if (clientSocketFileDescriptor == -1) {
                std::cerr << "[!] accept() failed to initialize: " << strerror(errno) << std::endl;
                continue;
            }

            std::lock_guard<std::mutex> lock(clientMutex);
            std::cout << "[*] New Client connected: FD " << clientSocketFileDescriptor << std::endl;

            std::thread t(&ClientSocket::handleClient, this, clientSocketFileDescriptor);
            t.detach();
            activeClientsSFD.push_back(clientSocketFileDescriptor);           
        }

        closeClientSocket();
        std::cout << "[*] Client FD: " << clientSocketFileDescriptor << " connection closed." << std::endl;
    }

    void closeClientSocket() {
        close(clientSocketFileDescriptor);
        return;
    }
};

class Server {
    private:
    ServerSocket serverSocket;
    ClientSocket clientSocket;    

    public:
    Server(){};
    ~Server() {
        serverSocket.closeServerSocket();
        clientSocket.closeClientSocket();        
    }

    void createServerSocket() {
        if(serverSocket.initialize() == false) {
            std::cerr << "[!] Failed to initialize server socket. Exiting." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    void createClientSocket() {
        clientSocket.initialize(serverSocket.getSSFD());
    }

    void listenForClients() {
        // Listen for incoming connections:
        if (listen(serverSocket.getSSFD(), 5) == -1) {
            std::cerr << "[!] listen() failed to initialize: " << strerror(errno) << std::endl;
            serverSocket.closeServerSocket();
            return;
        }

        std::cout << "Server listening on port 8080..." << std::endl;
    }
};

int main() {

    Server server;

    server.createServerSocket();
    server.listenForClients();
    server.createClientSocket();

    return 0;
}

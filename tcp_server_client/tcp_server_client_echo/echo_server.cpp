// === TCP ECHO SERVER ===
// This simple TCP server listens for a localhost client connection on port 8080.
// It then receives a client message which it then echoes back to the client.

#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Class for creation and binding of the server socket:
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

    void initialize() {
        serverSocketFileDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocketFileDescriptor == -1) {
            std::cerr << "[!] socket() failed to initialize: " << strerror(errno) << std::endl;
            return;
        }

        serverSocketAddress.sin_family = AF_INET;                  
        serverSocketAddress.sin_addr.s_addr = INADDR_ANY;          
        serverSocketAddress.sin_port = htons(8080);                

        if (bind(serverSocketFileDescriptor, (struct sockaddr*) &serverSocketAddress, serverSize) == -1) {
            std::cerr << "[!] bind() failed to initialize: " << strerror(errno) << std::endl;
            closeServerSocket();
            return;
        }
    }

    void closeServerSocket() {
        std::cout << "Shutting down server socket..." << std::endl;
        close(serverSocketFileDescriptor);
        return;
    }
};

// Class for creation of the client socket, accepting/receiving data from said
// client, and then echoing the client message back to the client:
class ClientSocket {
    private:
    int clientSocketFileDescriptor;
    struct sockaddr_in clientSocketAddress;
    socklen_t clientSize = sizeof(clientSocketAddress);

    public:
    ClientSocket(){};
    ~ClientSocket() {
        closeClientSocket();
    }

    void initialize(int SSFD) {
        while (true) {
            clientSocketFileDescriptor = accept(SSFD, (struct sockaddr*) &clientSocketAddress, &clientSize);
            if (clientSocketFileDescriptor == -1) {
                std::cerr << "[!] accept() failed to initialize: " << strerror(errno) << std::endl;
                continue; // Try to accept another client.
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientSocketAddress.sin_addr, clientIP, INET_ADDRSTRLEN);

            std::cout << "Client connected from " << clientIP << ":" << ntohs(clientSocketAddress.sin_port) << std::endl;

            // Receive data from client:
            char clientMessage[1024];
            char serverMessage[1024];
            memset(clientMessage, 0, sizeof(clientMessage));
            memset(serverMessage, 0, sizeof(serverMessage));

            int bytesReceived = recv(clientSocketFileDescriptor, clientMessage, sizeof(clientMessage) -1, 0);
            if (bytesReceived > 0) {
                std::cout << "Received " << bytesReceived << " bytes: " << clientMessage << std::endl;
                // Echo message back to client:
                strncpy(serverMessage, clientMessage, sizeof(clientMessage));
                int bytesSent = send(clientSocketFileDescriptor, serverMessage, sizeof(serverMessage), 0);
                if (bytesSent > 0) {
                    std::cout << "Sent " << bytesSent << " bytes: " << serverMessage << std::endl;
                } else {
                    std::cerr << "[!] Server send() failure: " << strerror(errno) << std::endl;
                }            
            } else if (bytesReceived == 0) {
                std::cout << "Client disconnected without sending data." << std::endl;
            } else {
                std::cerr << "[!] recv() failed to initialize: " << strerror(errno) << std::endl;
            }

            closeClientSocket();
            std::cout << "Connection closed." << std::endl;
        }
    }

    void closeClientSocket() {
        close(clientSocketFileDescriptor);
        return;
    }
};

// Class for the server, itself, utilizing the ServerSocket and ClientSocket classes:
class Server {
    public:
    ServerSocket serverSocket;
    ClientSocket clientSocket;

    Server(){};
    ~Server() {
        serverSocket.closeServerSocket();
        clientSocket.closeClientSocket();        
    }

    void createServerSocket() {
        serverSocket.initialize();
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

// Program entry:
int main() {

    Server server;

    server.createServerSocket();
    server.listenForClients();
    server.createClientSocket();

    return 0;
}

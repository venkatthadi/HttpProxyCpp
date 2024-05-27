#include "ProxyServer.h"
#include "MessageHandler.h"
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define PROXY_PORT 8080
#define MAX_SIZE 65536
#define BACKLOG 10

void handleClientThread(SOCKET client_sockfd) {
    ProxyServer proxyServer;
    proxyServer.handleClient(client_sockfd);
    closesocket(client_sockfd);
}

void ProxyServer::start() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cerr << "WSAStartup failed: " << result << endl;
        return;
    }

    SOCKET sockfd, newsockfd;
    struct sockaddr_in prox_addr, client_addr;
    int cli_len;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Server: socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }
    cout << "[+]socket created." << endl;

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes)) == SOCKET_ERROR) {
        cerr << "setsockopt failed: " << WSAGetLastError() << endl;
        closesocket(sockfd);
        WSACleanup();
        return;
    }

    memset(&prox_addr, 0, sizeof(prox_addr));
    prox_addr.sin_family = AF_INET;
    prox_addr.sin_port = htons(PROXY_PORT);
    prox_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&prox_addr, sizeof(prox_addr)) == SOCKET_ERROR) {
        cerr << "Server: bind failed: " << WSAGetLastError() << endl;
        closesocket(sockfd);
        WSACleanup();
        return;
    }
    cout << "[+]binded to port - " << PROXY_PORT << "." << endl;

    if (listen(sockfd, BACKLOG) == SOCKET_ERROR) {
        cerr << "Server: listen failed: " << WSAGetLastError() << endl;
        closesocket(sockfd);
        WSACleanup();
        return;
    }
    cout << "[+]listening..." << endl;

    cli_len = sizeof(client_addr);
    while (true) {
        newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &cli_len);
        if (newsockfd == INVALID_SOCKET) {
            cerr << "Server: accept failed: " << WSAGetLastError() << endl;
            continue;
        }
        cout << "[+]client accepted." << endl;

        thread clientThread(handleClientThread, newsockfd);
        clientThread.detach();
    }
    closesocket(sockfd);
    WSACleanup();
    cout << "[+]proxy closed." << endl;
}

void ProxyServer::handleClient(SOCKET client_sockfd) {
    int bytes_received;
    char buffer[MAX_SIZE];
    char newbuffer[MAX_SIZE];

    memset(buffer, 0, MAX_SIZE);
    bytes_received = recv(client_sockfd, buffer, MAX_SIZE, 0);
    if (bytes_received > 0) {
        strncpy_s(newbuffer, buffer, bytes_received);
        newbuffer[bytes_received] = '\0'; // Ensure null-termination
    }

    if (strncmp(buffer, "CONNECT", 7) == 0) {
        cout << "[+]request (connect) - " << endl << buffer << endl;
        char* next_token = nullptr;
        char* host = strtok_s(buffer + 8, ":", &next_token);
        int port = atoi(strtok_s(nullptr, " ", &next_token));
        cout << "[+]connect request for host - " << host << ", port - " << port << "." << endl;

        SOCKET server_sockfd;
        if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            cerr << "Server socket failed: " << WSAGetLastError() << endl;
            closesocket(server_sockfd);
            return;
        }
        cout << "[+]server socket created." << endl;

        struct addrinfo hints, * result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int res = getaddrinfo(host, nullptr, &hints, &result);
        if (res != 0) {
            cerr << "getaddrinfo failed: " << gai_strerror(res) << endl;
            closesocket(server_sockfd);
            closesocket(client_sockfd);
            return;
        }

        struct sockaddr_in* serv_addr = (struct sockaddr_in*)result->ai_addr;
        serv_addr->sin_port = htons(port);

        if (connect(server_sockfd, (struct sockaddr*)serv_addr, sizeof(*serv_addr)) == SOCKET_ERROR) {
            cerr << "Server: connect failed: " << WSAGetLastError() << endl;
            closesocket(server_sockfd);
            freeaddrinfo(result);
            return;
        }
        cout << "[+]server connected." << endl;

        freeaddrinfo(result);

        const char* connect_response = "HTTP/1.1 200 Connection established\r\n\r\n";
        int n = send(client_sockfd, connect_response, strlen(connect_response), 0);
        // cout << n << endl;
        cout << "[+]response sent." << endl;

        MessageHandler messageHandler;
        messageHandler.handleMessages(client_sockfd, server_sockfd);
    }
    else {
        char* host = strstr(buffer, "Host: ");
        char host_buffer[256];
        sscanf_s(host, "Host: %s", host_buffer, (unsigned)_countof(host_buffer));
        if (!host) {
            cerr << "Invalid request." << endl;
            return;
        }
        cout << "Request - " << host_buffer << endl;

        struct addrinfo hints, * result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int res = getaddrinfo(host_buffer, "80", &hints, &result);
        if (res != 0) {
            cerr << "getaddrinfo failed: " << gai_strerror(res) << endl;
            return;
        }

        SOCKET serv_sockfd;
        if ((serv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            cerr << "Server: socket failed: " << WSAGetLastError() << endl;
            freeaddrinfo(result);
            return;
        }
        cout << "[+]server socket created." << endl;

        if (connect(serv_sockfd, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
            cerr << "Server: connect failed: " << WSAGetLastError() << endl;
            closesocket(serv_sockfd);
            freeaddrinfo(result);
            return;
        }
        cout << "[+]server connected." << endl;

        freeaddrinfo(result);

        if (send(serv_sockfd, newbuffer, bytes_received, 0) == SOCKET_ERROR) {
            cerr << "Sending request failed: " << WSAGetLastError() << endl;
            closesocket(serv_sockfd);
            return;
        }
        cout << "[+]request sent to server." << endl;

        char response[MAX_SIZE];
        memset(response, 0, MAX_SIZE);
        if ((bytes_received = recv(serv_sockfd, response, MAX_SIZE, 0)) == SOCKET_ERROR) {
            cerr << "Receiving response failed: " << WSAGetLastError() << endl;
            closesocket(serv_sockfd);
            return;
        }
        cout << "[+]response received from server bytes - " << bytes_received << "." << endl;

        if (send(client_sockfd, response, bytes_received, 0) == SOCKET_ERROR) {
            cerr << "Sending response failed: " << WSAGetLastError() << endl;
            closesocket(serv_sockfd);
            return;
        }
        cout << "[+]response forwarded to client." << endl;

        closesocket(client_sockfd);
        cout << "[+]closed client." << endl;
        closesocket(serv_sockfd);
        cout << "[+]closed server." << endl;
    }
}
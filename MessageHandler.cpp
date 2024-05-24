#include "MessageHandler.h"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

void MessageHandler::handleMessages(SOCKET client_socket, SOCKET destination_socket) {
    fd_set readfds;
    char data_buffer[2048];
    int n;
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(client_socket, &readfds);
        FD_SET(destination_socket, &readfds);

        int max_fd = max(client_socket, destination_socket) + 1;
        int activity = select(max_fd, &readfds, NULL, NULL, NULL);

        if (activity == SOCKET_ERROR) {
            cerr << "Select failed: " << WSAGetLastError() << endl;
            return;
        }

        if (FD_ISSET(client_socket, &readfds)) {
            n = recv(client_socket, data_buffer, sizeof(data_buffer), 0);
            if (n <= 0) {
                return;
            }
            data_buffer[n] = '\0';
            send(destination_socket, data_buffer, n, 0);
        }

        if (FD_ISSET(destination_socket, &readfds)) {
            n = recv(destination_socket, data_buffer, sizeof(data_buffer), 0);
            if (n <= 0) {
                return;
            }
            data_buffer[n] = '\0';
            send(client_socket, data_buffer, n, 0);
        }
    }
}
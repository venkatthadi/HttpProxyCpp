#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <winsock2.h>

class MessageHandler {
public:
    void handleMessages(SOCKET client_socket, SOCKET destination_socket);
};

#endif
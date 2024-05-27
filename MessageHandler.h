#ifndef MESSAGE_HANDLER
#define MESSAGE_HANDLER

#include <winsock2.h>

class MessageHandler {
public:
    void handleMessages(SOCKET client_socket, SOCKET destination_socket);
};

#endif
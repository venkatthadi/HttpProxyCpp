#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <winsock2.h>

class ProxyServer {
public:
    void start();
    void handleClient(SOCKET client_sockfd);
};

#endif

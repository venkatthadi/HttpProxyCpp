#ifndef PROXY_SERVER
#define PROXY_SERVER

#include <winsock2.h>

class ProxyServer {
public:
    void start();
    void handleClient(SOCKET client_sockfd);
};

#endif

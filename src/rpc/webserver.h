#include <util/system.h>


class CWebServer
{

private:
    SOCKET listenSocket;

public:
    CWebServer();

    void InitSockets();

};

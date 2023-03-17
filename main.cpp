#include "server/server.h"
using namespace std;

int main()
{
    event_type mode1 = EPOLLIN;
    event_type mode2 = EPOLLET | EPOLLONESHOT | EPOLLIN;
    server web_server(12345, mode1, mode2, 8);
    web_server.run();
    return 0;
}
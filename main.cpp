// #include "server/server.h"
#include <string>
#include <iostream>
#include <memory>
#include <functional>
#include <queue>
#include <unistd.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <string>

#include "server/server.h"
using namespace std;

int main()
{
    event_type mode1 = EPOLLIN;
    event_type mode2 = EPOLLET | EPOLLONESHOT;
    server web_server(mode1, mode2, 1);
    web_server.run();
    return 0;
}
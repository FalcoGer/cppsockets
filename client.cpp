#include <iostream>

#include "TCPSocket.h"
#include "NetAddress.h"

auto main() -> int
{
    const Port PORT{4444};
    TCPSocket  server(NetAddress("127.0.0.1"), PORT, true);

    std::cout << *server.recv() << '\n';
    server.send("hello from client!");

    return 0;
}

#include <iostream>

#include "../TCPSocket.h"

auto main() -> int
{
    const CPPSockets::Port PORT{4444};
    CPPSockets::TCPSocket  server(CPPSockets::NetAddress("127.0.0.1"), PORT, true);

    std::cout << *server.recv() << '\n';
    server.send("hello from client!");

    return 0;
}

#include <iostream>

#include "../TCPSocket.h"

auto main() -> int
{
    const CPPSockets::Port PORT{4444};
    CPPSockets::TCPSocket  server(CPPSockets::NetAddress("::1"), PORT, true, CPPSockets::Socket::EAddressFamily::IPV6);

    std::cout << *server.recv() << '\n';
    server.send("hello from client!");

    return 0;
}

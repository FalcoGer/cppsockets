#include <iostream>

#include "../ListeningSocket.h"

auto main() -> int
{
    const CPPSockets::Port PORT{4444};
    CPPSockets::ListeningSocket sock(CPPSockets::NetAddress("::1"), PORT, true, CPPSockets::Socket::EAddressFamily::IPV6);

    auto            client = *sock.accept();
    sock.close();

    client.send("hello from server.");
    std::cout << *client.recv() << '\n';

    return 0;
}

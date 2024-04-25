#include <iostream>

#include "../ListeningSocket.h"

auto main() -> int
{
    const CPPSockets::Port PORT{4444};
    CPPSockets::ListeningSocket sock(CPPSockets::NetAddress("127.0.0.1"), PORT, true);

    auto            client = *sock.accept();
    sock.close();

    client.send("hello from server.");
    std::cout << *client.recv() << '\n';

    return 0;
}

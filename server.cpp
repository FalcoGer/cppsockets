#include <iostream>

#include "ListeningSocket.h"
#include "NetAddress.h"

auto main() -> int
{
    const Port PORT{4444};
    ListeningSocket sock(NetAddress("127.0.0.1"), PORT, true);

    auto            client = *sock.accept();
    sock.close();

    client.send("hello from server.");
    std::cout << *client.recv() << '\n';

    return 0;
}

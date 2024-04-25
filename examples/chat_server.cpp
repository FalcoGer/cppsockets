#include <iostream>
#include <string>
#include <vector>

#include "../ListeningSocket.h"

// NOLINTNEXTLINE(google-build-using-namespace) // Example code only.
using namespace CPPSockets;

auto main() -> int
{
    const NetAddress         BINDADDR("127.0.0.1");
    const Port               BINDPORT(4'444);

    std::vector<TCPSocket>   clients {};
    std::vector<std::string> messageQueue;

    auto                     sock = ListeningSocket(BINDADDR, BINDPORT, false);

    while (true)
    {
        auto newClient = sock.accept();
        if (newClient.has_value())
        {
            std::cout << *newClient << " connected.\n";
            newClient->setBlocking(false);
            newClient->send("Welcome to the chat.\n");
            messageQueue.push_back(
              std::format("{}:{} has joined.\n", newClient->getAddress().data(), newClient->getPort().data())
            );
            clients.push_back(std::move(*newClient));
        }
        for (auto& client : clients)
        {
            if (client.queryConnectionClosed())
            {
                std::cout << client << " disconnected.\n";
                messageQueue.push_back(
                  std::format("{}:{} has left.\n", client.getAddress().data(), client.getPort().data())
                );
                continue;
            }
            auto msg = client.recv();
            if (msg.has_value())
            {
                std::cout << client << ": " << *msg << '\n';
                messageQueue.push_back(
                  std::format("[ {}:{} ]: {}\n", client.getAddress().data(), client.getPort().data(), *msg)
                );
            }
        }

        std::erase_if(clients, [](TCPSocket& client) { return !client.isOpen(); });

        for (auto& client : clients)
        {
            for (auto& msg : messageQueue)
            {
                client.send(msg);
                if (!client.isOpen())
                {
                    break;
                }
            }
        }

        messageQueue.clear();
    }
}

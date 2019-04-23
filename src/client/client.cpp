#include <common/grass.h>
#include <common/common.h>
#include <common/threadpool.h>
#include <common/socket.h>
#include <client/cli.h>

#include <iostream>
#include <string>
#include <tuple>
#include <exception>

using std::cout;
using std::endl;

/**
 * Parse the command-line arguments.
 * @return The IP address and port number of the server.
 */
std::pair<std::string, uint16_t> parseArgs(int argc, char **argv) {
    if (argc < 3) {
        cout << "Usage:" << std::endl;
        cout << argv[0] << " server-ip server-port" << std::endl;
        throw std::invalid_argument("");
    } else {
        std::string server_ip(argv[1]);
        std::string server_port_s(argv[2]);

        int server_port_i = std::stoi(server_port_s);
        uint16_t server_port = 0;
        if (0 <= server_port_i && server_port_i <= static_cast<int>(UINT16_MAX)) {
            server_port = static_cast<uint16_t>(server_port_i);
            return make_pair(server_ip, server_port);
        } else {
            throw std::invalid_argument("Port number is too big");
        }
    }
}

int main(int argc, char **argv) {
    Cli cli;
    Socket socket;
    ThreadPool<int> tpool(2);

    try {
        auto args = parseArgs(argc, argv);
        Address address = Socket::parseAddress(args.first, args.second);
        socket.connect(address);

        // Read the input and send it continuously.
        tpool.schedule(1, [&](){
            while (true) socket << cli.readInput() << endl;
        });

        // Read from the socket and print continuously.
        tpool.schedule(2, [&](){
            while (true) {
                std::string packet;
                socket >> packet;
                cli.showError(packet);
            }
        });
    } catch (const NetworkingException& e) {
        cout << "[ERROR] Networking: " << e.what() << endl;
    } catch (const std::exception& e) {
        cout << "[ERROR] " << e.what() << endl;
    }

    sleep(1); // FIXME(liautaud)
    // Wait until all the threads are finished executing.
    tpool.join();

    return 0;
}

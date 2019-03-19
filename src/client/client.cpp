#include <common/grass.h>
#include <netinet/in.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <tuple>
#include <arpa/inet.h>
#include <exception>
#include <common/common.h>
#include <client/cli.h>

std::tuple<std::string, uint16_t> parseArgs(int argc, char **argv) {
    if (argc < 3) {
        std::cout<<"Usage:"<<std::endl;
        std::cout<<"client server-ip server-port"<<std::endl;
        throw std::invalid_argument("");
    } else {
        std::string server_ip(argv[1]);
        std::string server_port_s(argv[2]);

        int server_port_i = std::stoi(server_port_s);
        uint16_t server_port = 0;
        if (0 <= server_port_i && server_port_i <= static_cast<int>(UINT16_MAX)) {
            server_port = static_cast<uint16_t>(server_port_i);
            return {server_ip, server_port};
        } else {
            throw std::invalid_argument("Port number is too big");
        }
    }
}

void cli() {
    Cli cli;
    while (true) {
        cli.readInput();
    }
}

int main(int argc, char **argv) {
    uint16_t server_port;
    std::string server_ip;

    int server_socket = -1;
    try {
        std::tie(server_ip, server_port) = parseArgs(argc, argv);

        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(server_port);

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            throw NetworkingException("Couldn't create the socket");
        }

        if(inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr)<=0) {
            throw NetworkingException("Invalid address");
        }

        if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            throw NetworkingException("Connection failed");
        }

        std::string message = "hello world";
        send(server_socket, message.c_str(), message.size(), 0);

        cli();
    } catch (NetworkingException const &e) {
        std::cout<<"[Networking] "<<e.what()<<std::endl;
    } catch (std::exception const &e) {
        std::cout<<e.what()<<std::endl;
    }

    if (server_socket >= 0)
        close(server_socket);
    return 0;
}

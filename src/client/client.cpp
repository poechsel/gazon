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
#include <common/threadpool.h>

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

void execCli(Cli *cli, int server_socket) {
    while (true) {
        std::string command = cli->readInput();

        //TODO: Fix the send pattern
        send(server_socket, command.c_str(), command.size(), 0);
    }
}

void receiveCommunications(Cli *cli, int server_socket) {
    fd_set sockets_set;
    FD_ZERO(&sockets_set);
    FD_SET(server_socket, &sockets_set);
    while (true) {
        fd_set read_fd = sockets_set;
        int max_socket = server_socket;

        if (select(max_socket + 1, &read_fd, NULL, NULL, NULL) == -1) {
            throw NetworkingException("Select error");
        }

        if (FD_ISSET(server_socket, &read_fd)) {
            char buffer[1024];
            // TODO fix this
            int l = recv(server_socket, buffer, 1024, 0);
            buffer[l] = 0;
            if (l == 0) {
                // server got a problem
                cli->showError("The server stopped working");
                break;
            } else {
                cli->showError(std::string(buffer));
            }
        }
    }
}

int main(int argc, char **argv) {
    uint16_t server_port;
    std::string server_ip;

    ThreadPool<int> pool(2);
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

        Cli cli;
        pool.schedule(1, [&](){execCli(&cli, server_socket);});
        pool.schedule(2, [&](){receiveCommunications(&cli, server_socket);});
        // just to make sure that the cli started
        sleep(.5);
        // finally wait for the threads to end
        pool.join();
    } catch (NetworkingException const &e) {
        std::cout<<"[Networking] "<<e.what()<<std::endl;
    } catch (std::exception const &e) {
        std::cout<<e.what()<<std::endl;
    }

    if (server_socket >= 0)
        close(server_socket);
    return 0;
}

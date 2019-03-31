#include <common/grass.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <map>
#include <common/common.h>
#include <common/config.h>
#include <common/command.h>
#include <common/threadpool.h>

#include <errno.h>
#include <string.h>

#include <common/regex.h>
#include <common/filesystem.h>

struct Peer {
    int foo;
};

void connectAndListen(uint16_t port, int server_socket) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == 0) {
        throw NetworkingException("Couldn't create a socket");
    }

    int reuse_addr = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)) == -1) {
        throw NetworkingException("Couldn't set option on socket");
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw NetworkingException("Couldn't bind the socket");
    }

    if (listen(server_socket, SOMAXCONN) < 0) {
        throw NetworkingException("Couldn't listen on the socket");
    }

    std::map<int, Peer*> peers;

    fd_set sockets_set;
    FD_ZERO(&sockets_set);
    FD_SET(server_socket, &sockets_set);

    while (true) {
        fd_set read_fd = sockets_set;
        int max_socket = server_socket;

        for (auto& peer : peers) {
            max_socket = std::max(max_socket, peer.first);
        }

        if (select(max_socket + 1, &read_fd, NULL, NULL, NULL) == -1) {
            std::cout<<strerror(errno)<<"\n"<<std::flush;
            throw NetworkingException("Select error");
        }


        if (FD_ISSET(server_socket, &read_fd)) {
            struct sockaddr_in incoming_address;
            socklen_t len = sizeof(sockaddr_in);
            int new_socket = accept(server_socket, (struct sockaddr*)&incoming_address, &len);
            if (new_socket == -1) {
                std::cout<<"Couldn't accept a new connection"<<std::endl;
                // TODO: exit ?
            } else {
                std::cout<<"New connection from "<<inet_ntoa(incoming_address.sin_addr)
                         <<" on port " <<ntohs(incoming_address.sin_port)<<std::endl;
                FD_SET(new_socket, &sockets_set);
                max_socket = std::max(max_socket, new_socket);
                std::cout<<"new socket: "<<new_socket<<"\n"<<std::flush<<"\n";
                peers[new_socket] = new Peer();
                std::string message = "pong";
                send(new_socket, message.c_str(), message.size(), 0);
            }
        }

        std::vector<int> closed_sockets;
        for (auto const& peer : peers) {
            int socket = peer.first;
            if (FD_ISSET(socket, &read_fd)) {
                char buffer[1024];
                int l = recv(socket, buffer, 1024, 0);
                if (l == 0) {
                    closed_sockets.push_back(socket);
                } else if (l < 0) {
                    std::cout<<strerror(errno)<<"\n"<<std::flush;
                    closed_sockets.push_back(socket);
                } else {
                    buffer[l] = 0;
                    // TODO: execute the following in a thread
                    try {
                        // evaluateCommandFromLine(nullptr, std::string(buffer), true);
                    } catch (const std::exception& e) {
                        // TODO send back the error
                        std::cout<<e.what()<<"\n";
                    }
                }
            }
        }
        for (auto socket : closed_sockets) {
            FD_CLR(socket, &read_fd);
            delete peers[socket];
            peers.erase(socket);
            close(socket);
        }
    }
    close(server_socket);
}

int main() {
    /*Regex("ab(c+d|e*f|gh)?|poi?j");
    Regex("a{ 0, 4 }{ 1 }{ ,  3  }{ 5 , }");
    Regex("ab?(abc|def(A{4, 9}|x)?){ 4 }");
    Regex("ab{1,2}");
    Regex("ab{0,2}");
    Regex("ab{2,}");
    */
    //Path test("/~/abc/../foo/./../");
    Filesystem fs("/home/pierre/");
    fs.scanAll();
    std::cout<<"\n\n-------\n";
    //fs.debug(&Filesystem::root);
    std::cout<<"\n-------\n\n";
    Filesystem::ls();
    return 0;
    

    auto re = Regex("(a{2,3}|b){,5}");
    std::cout<<re.match("aaa")<<"\n";
    std::cout<<re.match("abbaaa")<<"\n";
    std::cout<<re.match("abbab")<<"\n";
    return 0;
    /*ThreadPool<int> pool(4);
  pool.schedule(1, [](){std::cout<<"foo\n"; sleep(3);});
  pool.schedule(1, [](){std::cout<<"baz\n"; sleep(1);});
  pool.schedule(2, [](){std::cout<<"bar\n"; sleep(1);});
  pool.schedule(3, [](){std::cout<<"bar\n"; sleep(1);});
  pool.schedule(4, [](){std::cout<<"bar\n"; sleep(1);});
  pool.schedule(5, [](){std::cout<<"bar\n"; sleep(1);});
  pool.schedule(6, [](){std::cout<<"bar\n"; sleep(1);});
  pool.schedule(7, [](){std::cout<<"bar\n"; sleep(1);});
  tool.schedule(8, [](){std::cout<<"bar\n"; sleep(1);});
  sleep(5);
  std::cout<<"joining threads\n";
  pool.join();
  return 0;
  */
    // Listen to the port and handle each connection
    int server_socket = -1;
    try {
        Config config = Config::fromFile("grass.conf");
        auto command = CommandFactory::create("hello");
        Socket socket;
        Context context;
        command->execute(socket, context, {"www.google.com"});
        delete(command);
        connectAndListen(config.port, server_socket);
        CommandFactory::destroy();
    } catch (NetworkingException const& e) {
        std::cout<<"[Networking] "<<e.what()<<std::endl;
    } catch (ConfigException const& e) {
        std::cout<<"[Config]"<<e.what()<<std::endl;
    }

    if (server_socket >= 0)
        close(server_socket);
}

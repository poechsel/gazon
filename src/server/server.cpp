#include <grass.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unordered_map>
#include <common.h>
#include <config.h>

static struct User **userlist;
static int numUsers;
static struct Command **cmdlist;
static int numCmds;

struct Peer {
    
};


// Helper function to run commands in unix.
void run_command(const char* command, int sock){
}


/*
 * Send a file to the client as its own thread
 *
 * fp: file descriptor of file to send
 * sock: socket that has already been created.
 */
void send_file(int fp, int sock) {
}

/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to save to.
 * sock: socket that has already been created.
 * size: the size (in bytes) of the file to recv
 */
void recv_file(int fp, int sock, int size) {
}

// Server side REPL given a socket file descriptor
void *connection_handler(void* sockfd) {
}

/*
 * search all files in the current directory
 * and its subdirectory for the pattern
 *
 * pattern: an extended regular expressions.
 * Output: A line seperated list of matching files' addresses
 */
void search(char *pattern) {
    // TODO
}

// Parse the grass.conf file and fill in the global variables
void parse_grass() {
}

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

    std::unordered_map<int, Peer> peers;

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
                peers[new_socket] = Peer();
            }

        }

        for (auto& peer : peers) {
            int socket = peer.first;
            if (FD_ISSET(socket, &read_fd)) {
                char buffer[1024];
                int l = recv(socket, buffer, 1024, 0);
                if (l == 0) {
                    FD_CLR(socket, &read_fd);
                    close(socket);
                    peers.erase(socket);
                } else {
                    buffer[l] = 0;
                    std::cout<<">> "<<l<<" : "<<buffer<<"\n";
                }
            }
        }
    }
    close(server_socket);
}

int main() {
    // TODO:
    // Parse the rass.conf file
    // Listen to the port and handle each connection
    int server_socket = -1;
    try {
        Config config = Config::fromFile("grass.conf");
        connectAndListen(config.port, server_socket);
    } catch (NetworkingException const& e) {
        std::cout<<"[Networking] "<<e.what()<<std::endl;
    } catch (ConfigException const& e) {
        std::cout<<"[Config]"<<e.what()<<std::endl;
    }
    
    if (server_socket >= 0)
        close(server_socket);
}

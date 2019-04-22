#include <common/socket.h>

#include <vector>
#include <iostream>
#include <algorithm>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

using std::cout;
using std::endl;

/** Return a formatted string containing the current value of errno. */
std::string formatError(const string& message) {
    std::stringstream error;
    error << message << ": " << ::strerror(errno) << ".";
    return error.str();
}

/** Create a new socket. */
Socket::Socket() {
    if ((fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw NetworkingException("Couldn't create a new socket.");
    }
}

/** Connect the socket to a remote address. */
void Socket::connect(const Address& address) {
    if (fd < 0) {
        throw NetworkingException("Could not connect: socket is uninitialized.");
    }

    if (::connect(fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        throw NetworkingException(formatError("Could not connect"));
    }
}

/** Bind the socket to a local address. */
void Socket::bind(const Address& address) {
    if (fd < 0) {
        throw NetworkingException("Could not bind: socket is uninitialized.");
    }

    if (::bind(fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        throw NetworkingException(formatError("Could not bind"));
    }
}

/** Listen for new connections on the socket. */
ConnectionPool Socket::listen() {
    if (fd < 0) {
        throw NetworkingException("Could not listen: socket is uninitialized.");
    }

    if (::listen(fd, SOMAXCONN) < 0) {
        throw NetworkingException(formatError("Could not listen"));
    }

    return ConnectionPool(fd);
}

/** Write an arbitrary output stream to the socket. */
Socket &Socket::operator<<(std::ostream&(*f)(std::ostream&)) {
    std::ostringstream s;
    s << f;
    write(s.str().c_str());
    return *this;
}

/** Write a string to the socket. */
Socket &Socket::operator<<(const string &s) {
    write(s.c_str());
    return *this;
}

/** Write a C-style string to the socket. */
Socket &Socket::operator<<(const char* s) {
    write(s);
    return *this;
}

/** Write a C-style string to the socket. */
void Socket::write(const char* s) {
    if (fd < 0) {
        throw NetworkingException("Could not write: socket is uninitialized.");
    }

    if (::write(fd, s, strlen(s)) < 0) {
        throw NetworkingException(formatError("Could not write"));
    }
}

/** Read a line from the socket. Blocks until EOL. **/
Socket& Socket::operator>>(string& destination) {
    if (fd < 0) {
        throw NetworkingException("Could not read: socket is uninitialized.");
    }

    char buffer[BUFFER_SIZE];
    size_t breakPosition;
    ssize_t bytesRead;

    // Repeatedly read from the socket until reading a line break.
    while ((breakPosition = received.find_first_of('\n')) == string::npos) {
        bytesRead = ::read(fd, &buffer, BUFFER_SIZE);

        if (bytesRead < 0) {
            throw NetworkingException(formatError("Could not read"));
        } else if (bytesRead == 0) {
            // There are no more bytes to read (usually because of EOF).
            break;
        }

        received.append(buffer, bytesRead);
    }

    // If there is nothing left to read, the connection is closed.
    if (received.length() == 0) {
        throw DisconnectException();
    }

    // Extract the first line, and keep the rest in a buffer.
    destination = received.substr(0, breakPosition);
    received.erase(0, breakPosition + 1);
    return *this;
}

/** Attempt to close the underlying TCP socket. */
void Socket::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

/** Convert a string and port to an IPv4 address. */
Address Socket::parseAddress(const string& ip, unsigned int port) {
    Address address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    
    switch (::inet_pton(AF_INET, ip.c_str(), &address.sin_addr)) {
        case -1:
            throw NetworkingException("Couldn't parse address (EAFNOSUPPORT).");
        case 0:
            throw NetworkingException("Couldn't parse address (bad format).");
        default:
            return address;
    }
}

/** Create a file descriptor set for all the active connections. */
std::pair<fd_set, int> ConnectionPool::makeDescriptorSet() {
    fd_set set;
    FD_ZERO(&set);

    // Add the local socket to the set.
    // This is needed so that select() notifies us of new connections.
    FD_SET(fd, &set);
    int maximum = fd;

    // Add all the connections to the set.
    for (auto const& connection : active) {
        FD_SET(connection.first, &set);
        maximum = std::max(connection.first, maximum);
    }

    return std::make_pair(set, maximum);
}

/** Run the event loop of the connection pool. */
void ConnectionPool::run() {
    fd_set set;
    int maximum;

    while (true) {
        std::vector<int> closed;
        std::tie(set, maximum) = makeDescriptorSet();

        // Block until one of the sockets is ready.
        if (select(maximum + 1, &set, 0, 0, 0) < 0) {
            throw NetworkingException(formatError("Could not select"));
        }

        // Check for new connections to accept.
        if (FD_ISSET(fd, &set)) {
            struct sockaddr_in incomingAddress;
            socklen_t len = sizeof(sockaddr_in);

            // We use accept4() to make sure that the call does not block, which
            // might happen if a client closes the connection between the time
            // of the select() and the time of the accept().
            int incomingSocket = accept4(
                fd, (struct sockaddr *) &incomingAddress, &len, SOCK_NONBLOCK
            );
            
            if (incomingSocket >= 0 && active.count(incomingSocket) == 0) {
                cout << "[INFO] New connection from "
                     << inet_ntoa(incomingAddress.sin_addr) << ":"
                     << ntohs(incomingAddress.sin_port) << endl;

                active.emplace(incomingSocket, incomingSocket);

                // Call the new connection handler if it has been set.
                if (onConnection) {
                    onConnection(active.at(incomingSocket));
                }
            } else if (incomingSocket >= 0) {
                cout << "[INFO] Existing connection from "
                     << inet_ntoa(incomingAddress.sin_addr) << ":"
                     << ntohs(incomingAddress.sin_port)
                     << ", ignoring." << endl;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                cout << "[INFO] Client aborted connection." << endl;
            } else {
                cout << "[ERROR] " << formatError("Could not accept") << endl;
            }
        }

        // Check for incoming data on existing connections.
        for (auto &connection : active) {
            int cfd = connection.first;

            if (FD_ISSET(cfd, &set)) {
                try {
                    // Check if the connection has been closed.
                    char buffer[1];
                    if (recv(cfd, buffer, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
                        throw DisconnectException();
                    }

                    // Call the incoming data handler if it has been set.
                    // It might raise a DisconnectException which will be caught.
                    if (onIncoming) {
                        onIncoming(connection.second);
                    }
                } catch (const DisconnectException& e) {
                    closed.push_back(cfd);
                }
            }
        }

        // Remove the closed sockets from the active set.
        // This will call the destructor of each removed Socket.
        for (int cfd : closed) {
            active.erase(cfd);
        }
    }
}
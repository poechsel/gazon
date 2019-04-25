#include <common/socket.h>

#include <vector>
#include <iostream>
#include <algorithm>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
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

    // This makes it simpler to restart the server after a crash.
    int reuse = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof(int)) < 0) {
        throw NetworkingException("Couldn't set REUSEADDR on the socket.");
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

/** Buffers data from the socket into the internal buffer. */
ssize_t Socket::buffer() {
    if (fd < 0) {
        throw NetworkingException("Could not buffer: socket is uninitialized.");
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead = ::recv(fd, &buffer, BUFFER_SIZE, 0);

    if (bytesRead < 0) {
        throw NetworkingException(formatError("Could not read"));
    }

    received.append(buffer, bytesRead);
    return bytesRead;
}

/** Try to extract a line from the socket's buffer. */
bool Socket::getLine(string& destination) {
    size_t breakPosition = received.find_first_of('\n');
    if (breakPosition == string::npos) {
        return false;
    }

    // Extract the first line, and keep the rest in the buffer.
    destination = received.substr(0, breakPosition);
    received.erase(0, breakPosition + 1);
    return true;
}

/** Read a line from the socket. Blocks until EOL. */
Socket& Socket::operator>>(string& destination) {
    if (fd < 0) {
        throw NetworkingException("Could not read: socket is uninitialized.");
    }

    // Repeatedly read from the socket until reading a line break.
    while (!getLine(destination)) {
        if (buffer() == 0) {
            // If there are no more bytes to read (usually because of EOF),
            // we will never be able to read a full line again.
            throw DisconnectException();
        }
    }

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

    // Make sure the listening socket is non-blocking, otherwise the call to
    // accept() might block if a client closes the connection between the
    // time of the select() and the time of the accept().
    fcntl(fd, F_SETFL, O_NONBLOCK);

    while (true) {
        std::vector<int> closed;
        std::tie(set, maximum) = makeDescriptorSet();

        // Block until one of the sockets is ready.
        if (select(maximum + 1, &set, 0, 0, 0) < 0) {
            cout << "[ERROR] " << formatError("Could not select") << endl;
        }

        // Check for new connections to accept.
        if (FD_ISSET(fd, &set)) {
            struct sockaddr_in incomingAddr;
            socklen_t len = sizeof(sockaddr_in);

            int incomingFd = accept(fd, (struct sockaddr *) &incomingAddr, &len);

            if (incomingFd >= 0 && active.count(incomingFd) == 0) {
                cout << "[INFO] New connection from "
                     << inet_ntoa(incomingAddr.sin_addr) << ":"
                     << ntohs(incomingAddr.sin_port) << "." << endl;

                active.emplace(incomingFd, incomingFd);

                // Call the new connection handler if it has been set.
                if (onConnection) {
                    onConnection(active.at(incomingFd));
                }
            } else if (incomingFd >= 0) {
                cout << "[INFO] Existing connection from "
                     << inet_ntoa(incomingAddr.sin_addr) << ":"
                     << ntohs(incomingAddr.sin_port)
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
            Socket &socket = connection.second;

            if (FD_ISSET(cfd, &set)) {
                try {
                    // Store the incoming data in the socket's internal buffer,
                    // and check if the connection has been closed.
                    if (socket.buffer() == 0) {
                        throw DisconnectException();
                    }

                    // Check if the socket contains a full packet, in which case we
                    // call the packet handler. It might raise a DisconnectException.
                    std::string packet;
                    if (socket.getLine(packet) && onPacket) {
                        onPacket(socket, std::move(packet));
                    }
                } catch (const DisconnectException& e) {
                    if (onClosing) {
                        onClosing(socket);
                    }

                    cout << "[INFO] Client closed connection." << endl;
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
#pragma once

#include <common/common.h>
#include <sstream>
#include <memory>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

// X Create socket from scratch.
// X Create socket from file descriptor.
//
// X Bind socket to a local address.
// X Listen for new connections on the socket.
// OR
// X Connect socket to a remote address.
//
// - Select from a collection of sockets.
// - Send one line of data to a socket.
//   socket << Packet("login", username);
// - Receive one line of data from a socket.

using std::string;
using std::unique_ptr;
using Address = struct sockaddr_in;

/// Number of bytes to read on each call to ::read().
const int BUFFER_SIZE = 256;

/** Return a formatted string containing the current value of errno. */
std::string formatError(const string& message) {
    std::stringstream error;
    error << message << ": " << ::strerror(errno) << ".";
    return error.str();
}

/**
 * Collection of sockets connected to a local socket.
 * 
 * This is used for TCP servers which listen() for incoming connections on a
 * local socket. This abstraction takes care of cleaning up closed sockets,
 * as well as select()-ing sockets which are ready to be read from.
 */
class ConnectionPool {
public:
    /**
     * Create a connection pool from an existing file descriptor.
     * 
     * The file descriptor must reference a valid TCP socket, which must be
     * in the listening state (i.e. listen() must have been called before).
     */
    ConnectionPool(int fd) : fd(fd) {} 

private:
    int fd = -1; ///< The descriptor of the listening TCP socket.
};

/**
 * C++ abstraction over plain TCP sockets.
 *
 * This abstraction makes it easier to use TCP sockets inside C++ code, notably
 * by overloading stream operators and providing utility functions for reading
 * newline-terminated packets one by one.
 */
class Socket {
public:
    /** Create a new socket. */
    Socket() {
        if ((fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            throw NetworkingException("Couldn't create a new socket.");
        }

        // int reuse = 1;
        // if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof(int)) < 0) {
        //     throw NetworkingException("Couldn't set REUSEADDR on the socket.");
        // }
    }

    /** Create a socket from an existing file descriptor. */
    Socket(int fd) : fd(fd) {} 

    /** Destroy the socket (closes the underlying TCP socket). */
    ~Socket() {
        close();
    }

    /** Connect the socket to a remote address. */
    void connect(const Address& address) {
        if (fd < 0) {
            throw NetworkingException("Could not connect: socket is uninitialized.");
        }

        if (::connect(fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
            throw NetworkingException(formatError("Could not connect"));
        }
    }

    /** Bind the socket to a local address. */
    void bind(const Address& address) {
        if (fd < 0) {
            throw NetworkingException("Could not bind: socket is uninitialized.");
        }

        if (::bind(fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
            throw NetworkingException(formatError("Could not bind"));
        }
    }

    /**
     * Listen for new connections on the socket.
     * Returns a ConnectionPool to help keeping track of the connections.
     */
    ConnectionPool listen() {
        if (fd < 0) {
            throw NetworkingException("Could not listen: socket is uninitialized.");
        }

        if (::listen(fd, SOMAXCONN) < 0) {
            throw NetworkingException(formatError("Could not listen"));
        }

        return ConnectionPool(fd);
    }

    /** Write an arbitrary output stream to the socket. */
    Socket &operator<<(std::ostream&(*f)(std::ostream&)) {
        std::ostringstream s;
        s << f;
        write(s.str().c_str());
        return *this;
    }

    /** Write a string to the socket. */
    Socket &operator<<(const string &s) {
        write(s.c_str());
        return *this;
    }

    /** Write a C-style string to the socket. */
    Socket &operator<<(const char* s) {
        write(s);
        return *this;
    }

    /** Write a C-style string to the socket. */
    void write(const char* s) {
        if (fd < 0) {
            throw NetworkingException("Could not write: socket is uninitialized.");
        }

        if (::write(fd, s, strlen(s)) < 0) {
            throw NetworkingException(formatError("Could not write"));
        }
    }

    /** Read a line from the socket. Blocks until EOL. **/
    Socket& operator>>(string& destination) {
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

        // Extract the first line, and keep the rest in a buffer.
        destination = received.substr(0, breakPosition);
        received.erase(0, breakPosition + 1);
        return *this;
    }

    /**
     * Attempt to close the underlying TCP socket.
     * 
     * We don't check whether ::close() returns 0 since this method is called inside
     * the destructor, and throwing exceptions inside the destructor is generally a
     * very bad idea.
     */
    void close() {
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }

    /** Convert a string and port to an IPv4 address. */
    static Address parseAddress(const string& ip, unsigned int port) {
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

private:
    int fd = -1;     ///< The descriptor of the underlying TCP socket.
    string received; ///< An internal buffer for the received data.
};
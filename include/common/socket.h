#pragma once

#include <common/common.h>
#include <set>
#include <sstream>
#include <functional>

using std::string;
using Address = struct sockaddr_in;

/// Number of bytes to read on each call to ::read().
const int BUFFER_SIZE = 256;

/// Forward declaration of ConnectionPool.
class ConnectionPool;

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
    Socket();

    /** Create a socket from an existing file descriptor. */
    Socket(int fd) : fd(fd) {} 

    /** Destroy the socket (closes the underlying TCP socket). */
    ~Socket() {
        close();
    }

    /** Connect the socket to a remote address. */
    void connect(const Address& address);

    /** Bind the socket to a local address. */
    void bind(const Address& address);

    /**
     * Listen for new connections on the socket.
     * Returns a ConnectionPool to help keeping track of the connections.
     */
    ConnectionPool listen();

    /** Write an arbitrary output stream to the socket. */
    Socket &operator<<(std::ostream&(*f)(std::ostream&));

    /** Write a string to the socket. */
    Socket &operator<<(const string &s);

    /** Write a C-style string to the socket. */
    Socket &operator<<(const char* s);

    /** Write a C-style string to the socket. */
    void write(const char* s);

    /** Read a line from the socket. Blocks until EOL. **/
    Socket& operator>>(string& destination);

    /**
     * Attempt to close the underlying TCP socket.
     * 
     * We don't check whether ::close() returns 0 since this method is called inside
     * the destructor, and throwing exceptions inside the destructor is generally a
     * very bad idea.
     */
    void close();

    /** Convert a string and port to an IPv4 address. */
    static Address parseAddress(const string& ip, unsigned int port);

private:
    friend class ConnectionPool;
    int fd = -1;     ///< The descriptor of the underlying TCP socket.
    string received; ///< An internal buffer for the received data.
};


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

    /**
     * Run the event loop of the connection pool.
     * 
     * This call is blocking: it starts an infinite loop which waits for
     * either a new connection to accept(), or a packet to receive. Note
     * that there is no guarantee about the actual implementation of the
     * event loop: it uses select() at the moment, but might change.
     */
    void run();

    /** Remove a socket from the connection pool. */
    void remove(const Socket& socket);

private:
    int fd = -1;             ///< The descriptor of the listening socket.
    std::set<Socket> active; ///< The set of active connections.

    /**
     * Create a file descriptor set for all the active connections.
     * Returns the maximum file descriptor as well, for use in select().
     */
    std::pair<fd_set, int> makeDescriptorSet();

    /// The handler used when a new connection is added to the pool.
    std::function<void(const Socket&)> onConnection;

    /// The handler used when a connection has new data incoming.
    std::function<void(const Socket&)> onIncoming;
};
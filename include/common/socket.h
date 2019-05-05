#pragma once

#include <common/common.h>
#include <unordered_map>
#include <functional>

#include <arpa/inet.h>

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
    ~Socket() { closeFd(); }

    /** Return the underlying file descriptor of the socket. */
    int getFd() const { return fd; }

    /** Return whether the socket is dirty. */
    bool isDirty() const { return dirty; }

    /** Connect the socket to a remote address. */
    void connect(const Address& address);

    /** Bind the socket to a local address. */
    void bind(const Address& address);

    /** Return the address that the socket is currently bound to. */
    Address getAddress() const;

    /**
     * Listen for new connections on the socket.
     * @return ConnectionPool to help keeping track of the connections.
     */
    ConnectionPool listen();

    /**
     * Write an exception to the socket.
     *
     * If the exception can't be written to the socket, fail silently
     * instead of throwing a NetworkingException.
     */
    Socket &operator<<(const std::exception &e);

    /** Write an arbitrary output stream to the socket. */
    Socket &operator<<(std::ostream&(*f)(std::ostream&));

    /** Write a string to the socket. */
    Socket &operator<<(const string &s);

    /** Write a C-style string to the socket. */
    Socket &operator<<(const char* s);

    /** Write a C-style string to the socket. */
    void write(const char* s);

    /**
     * Write raw data to the socket.
     *
     * @param buffer The source buffer for the data.
     * @param count  The number of bytes to write.
     */
    void write(const char* buffer, unsigned int count);

    /**
     * Buffer data from the socket into the internal buffer.
     * @return Number of bytes that were buffered.
     */
    ssize_t buffer();

    /**
     * Try to extract a line from the socket's buffer.
     * @return Whether destination was actually rewritten with a line.
     */
    bool getLine(string& destination);

    /** Read a line from the socket. Blocks until EOL. */
    Socket& operator>>(string& destination);

    /**
     * Close the socket.
     *
     * - In normal mode, calls closeFd() to close the underlying file descriptor.
     * - In deffered mode, only marks the socket as "dirty". The actual call to
     *   closeFd() will only be ade inside the destructor.
     */
    void close();

    /** Enable the deffered close mode on the socket. */
    void useDeferredClose();

    /** Throw an exception after closing the socket. */
    void useThrowOnClose();

    /** Convert a string and port to an IPv4 address. */
    static Address parseAddress(const string& ip, unsigned int port);

private:
    int fd = -1;                ///< The descriptor of the underlying TCP socket.
    string received;            ///< An internal buffer for the received data.
    bool deferredClose = false; ///< Whether to use deferred closing mode.
    bool throwOnClose = false;  ///< Whether to use throw when closing.
    bool dirty = false;         ///< Whether the socket is dirty.

    /**
     * Attempt to close the underlying file descriptor.
     *
     * We don't check whether ::close() returns 0 since this method is called inside
     * the destructor, and throwing exceptions inside the destructor is a bad idea.
     */
    void closeFd();
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

    /** Sets the onConnection handler. */
    void setOnConnection(std::function<void(Socket&)> handler) {
        onConnection = handler;
    }

    /** Sets the onIncoming handler. */
    void setOnPacket(std::function<void(Socket&, const std::string&)> handler) {
        onPacket = handler;
    }

    /** Sets the onClosing handler. */
    void setOnClosing(std::function<void(Socket&)> handler) {
        onClosing = handler;
    }

    /**
     * Run the event loop of the connection pool.
     *
     * This call is blocking: it starts an infinite loop which waits for
     * either a new connection to accept(), or a packet to receive. Note
     * that there is no guarantee about the actual implementation of the
     * event loop: it uses select() at the moment, but might change.
     */
    void run();

private:
    /// The descriptor of the listening socket.
    int fd = -1;

    /// The set of active connections.
    std::unordered_map<int, Socket> active;

    /// The handler used when a new connection is added to the pool.
    std::function<void(Socket&)> onConnection;

    /// The handler used when a connection has a new packet incoming.
    std::function<void(Socket&, const std::string&)> onPacket;

    /// The handler used when a connection is closing.
    std::function<void(Socket&)> onClosing;

    /**
     * Create a file descriptor set for all the active connections.
     * @return Created fd_set and maximum file descriptor.
     */
    std::pair<fd_set, int> makeDescriptorSet();
};

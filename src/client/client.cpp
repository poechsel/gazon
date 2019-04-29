#include <common/grass.h>
#include <common/common.h>
#include <common/threadpool.h>
#include <common/socket.h>

#include <sys/socket.h>

#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <exception>
#include <thread>
#include <chrono>
#include <regex>

using namespace std;
using Pool = ThreadPool<int, 2>;
using InputFn = function<bool(string&)>;
using OutputFn = function<void(const string&)>;

class CliArguments {
public:
    std::string serverIp;
    uint16_t serverPort;
    std::ifstream inputStream;
    std::ofstream outputStream;

    /** Parse the command-line arguments. */
    CliArguments(int argc, char **argv) {
        if (argc != 3 && argc != 5) {
            cout << "Usage: " << argv[0] << " server-ip server-port";
            cout << " [in-file out-file]" << endl;
            throw std::invalid_argument("Incorrect arguments.");
        }

        if (argc == 5) {
            inputStream.open(argv[3]);
            if (!inputStream.is_open())
                throw std::invalid_argument("Input file is invalid.");

            outputStream.open(argv[4]);
            if (!outputStream.is_open())
                throw std::invalid_argument("Output file is invalid.");
        }

        serverIp = argv[1];
        int tempServerPort = std::stoi(std::string(argv[2]));
        if (tempServerPort < 0 || tempServerPort > static_cast<int>(UINT16_MAX))
            throw std::invalid_argument("Port number is too big.");
        serverPort = static_cast<uint16_t>(tempServerPort);
    }
};

/** Print a prompt and read a line from stdin. */
bool readInput(string &packet) {
    cout << "> " << flush;
    return static_cast<bool>(getline(cin, packet));
}

/** Print the output of a command to stdout. */
void printOutput(const string &packet) {
    cout << packet << endl;
}

/** Receive a file at the given path. */
void receiveFile(string path, string ip, unsigned int port, int size) {
    int ffd = ::socket(AF_INET, SOCK_STREAM, 0);
    Address addr = Socket::parseAddress(ip, port);
    enforce(connect(ffd, (struct sockaddr *) &addr, sizeof(addr)));

    ofstream output(path, ios::out | ios::binary | ios::trunc);
    char buffer[4096];
    int bytesRead;
    int totalRead = 0;
    while ((bytesRead = read(ffd, buffer, 4096)) > 0 && totalRead < size) {
        totalRead += bytesRead;
        output.write(buffer, bytesRead);
    }

    if (totalRead == size) {
        cout << "[INFO] Received file " << path << "." << endl;
    } else {
        cout << "[ERROR] Did not receive the whole file for "
             << path << "." << endl;
    }

    close(ffd);
    output.close();
}

/** Send a file at the given path. */
void sendFile(string path, string ip, unsigned int port) {
    int ffd = ::socket(AF_INET, SOCK_STREAM, 0);
    Address addr = Socket::parseAddress(ip, port);
    enforce(connect(ffd, (struct sockaddr *) &addr, sizeof(addr)));

    ifstream input(path, ios::in | ios::binary);
    char buffer[4096];
    while (true) {
        input.read(buffer, 4096);
        if (input.gcount() == 0) break;
        enforce(write(ffd, buffer, input.gcount()));
    }

    cout << "[INFO] Sent file " << path << "." << endl;
    close(ffd);
    input.close();
}

static string currentGet(""); // FIXME(liautaud): Replace with a queue.
static string currentPut(""); // FIXME(liautaud): Replace with a queue.

/** Run the client with given input and output streams. */
void run(
    const CliArguments &args, Socket &socket, Pool &pool,
    const InputFn &input, const OutputFn &output
) {
    // Read the input and send it continuously.
    pool.schedule(1, [&](){
        string p;
        regex getRegex("get ([^[:space:]]+)");
        regex putRegex("put ([^[:space:]]+) [[:digit:]]+");
        smatch match;

        while (input(p)) {
            // When we request a get or a put, we must store the corresponding
            // filename to a local queue so that we can retrieve it later when
            // we receive the get: or put: reply from the server.
            if (regex_search(p, match, getRegex)) {
                currentGet = match[1];
            } else if (regex_search(p, match, putRegex)) {
                currentPut = match[1];
            }

            socket << p << endl;
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    });

    // Read from the socket and print continuously.
    pool.schedule(2, [&](){
        string p;
        regex getRegex("get port: ([[:digit:]]+) size: ([[:digit:]]+)");
        regex putRegex("put port: ([[:digit:]]+)");
        smatch match;

        while (true) {
            socket >> p;

            if (regex_search(p, match, getRegex)) {
                unsigned int port = static_cast<unsigned int>(stoul(match[1]));
                int size = stoi(match[2]);
                thread(receiveFile, currentGet, args.serverIp, port, size).detach();
            } else if (regex_search(p, match, putRegex)) {
                unsigned int port = static_cast<unsigned int>(stoul(match[1]));
                thread(sendFile, currentPut, args.serverIp, port).detach();
            } else {
                output(p);
            }
        }
    });
}

/** Start the client. */
int main(int argc, char **argv) {
    Pool pool;

    try {
        CliArguments args(argc, argv);
        Socket socket;
        socket.connect(Socket::parseAddress(args.serverIp, args.serverPort));
        socket.useThrowOnClose();

        // If an infile and outfile were passed, run in testing mode.
        if (args.inputStream.is_open() && args.outputStream.is_open()) {
            run(args, socket, pool, [&args](string &packet) {
                return static_cast<bool>(getline(args.inputStream, packet));
            }, [&args](const string &packet) {
                args.outputStream << packet << endl;
            });
        } else {
            run(args, socket, pool, readInput, printOutput);
        }
    } catch (const NetworkingException& e) {
        cout << "[ERROR] Networking: " << e.what() << endl;
    } catch (const exception& e) {
        cout << "[ERROR] " << e.what() << endl;
    }

    // Wait until all the threads are finished executing.
    pool.join();

    return 0;
}

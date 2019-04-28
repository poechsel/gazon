#include <common/grass.h>
#include <common/common.h>
#include <common/threadpool.h>
#include <common/socket.h>

#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <exception>
#include <thread>
#include <chrono>

using std::cout;
using std::endl;
using Pool = ThreadPool<int, 2>;

class CliArguments {
public:
    std::string serverIp;
    uint16_t serverPort;
    std::ifstream inputStream;
    std::ofstream outputStream;

    /** Parse the command-line arguments. */
    void parseArgs(int argc, char **argv) {
        if (!(argc == 3 || argc == 5)) {
            cout << "Usage: " << argv[0] << " server-ip server-port";
            cout << " [in-file out-file]" << endl;
            throw std::invalid_argument("Incorrect arguments.");
        }

        if (argc == 5) {
            inputStream.open(argv[3]);
            outputStream.open(argv[4]);

            if (!inputStream.is_open())
                throw std::invalid_argument("Input file is invalid.");
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

/** Print a prompt and read a line from cin. */
std::string readInput() {
    cout << "> " << std::flush;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

/** Run the client in interactive mode. */
void runInteractive(CliArguments&, Socket &socket, Pool &pool) {
    // Read the input and send it continuously.
    pool.schedule(1, [&](){
        while (true) {
            socket << readInput() << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // Read from the socket and print continuously.
    pool.schedule(2, [&](){
        std::string packet;
        while (true) {
            socket >> packet;
            cout << packet << endl;
        }
    });
}

/** Run the client in automated testing mode. */
void runTesting(CliArguments& args, Socket &socket, Pool &pool) {
    // Read the input and send it continuously.
    pool.schedule(1, [&](){
        std::string line;
        while (std::getline(args.inputStream, line)) {
            std::cout<<"trying\n";
            cout << "[INFO] Sending packet `" << line << "`." << endl;
            socket << line << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    // Read from the socket and print continuously.
    pool.schedule(2, [&](){
        std::string packet;
        while (true) {
            socket >> packet;
            args.outputStream << packet << endl << std::flush;
        }
    });
}

/** Start the client. */
int main(int argc, char **argv) {
    Socket socket;
    Pool pool;
    CliArguments args;

    try {
        args.parseArgs(argc, argv);
        socket.connect(Socket::parseAddress(args.serverIp, args.serverPort));
        socket.useThrowOnClose();

        // If an infile and outfile were passed, run in testing mode.
        if (args.inputStream.is_open() && args.outputStream.is_open()) {
            runTesting(args, socket, pool);
        } else {
            runInteractive(args, socket, pool);
        }
    } catch (const NetworkingException& e) {
        cout << "[ERROR] Networking: " << e.what() << endl;
    } catch (const std::exception& e) {
        cout << "[ERROR] " << e.what() << endl;
    }

    // Wait until all the threads are finished executing.
    pool.join();

    return 0;
}

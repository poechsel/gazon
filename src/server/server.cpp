#include <common/grass.h>
#include <common/common.h>
#include <common/config.h>
#include <common/command.h>
#include <common/threadpool.h>
#include <common/filesystem.h>

#include <signal.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>

using std::cout;
using std::endl;

void run(Socket &server, std::unordered_map<int, Context> &contexts,
         std::mutex &contexts_mutex,
         ThreadPool<int, 4> &tpool, ThreadPool<FileKeyThread, 8> &fpool) {
    try {
        Config::fromFile("grass.conf");
        Filesystem::scan(Config::base_directory);
        Address address = Socket::parseAddress("127.0.0.1", Config::port);
        server.bind(address);
        ConnectionPool cpool = server.listen();

        cout << " .d88b.   8888b.  88888888  .d88b.  88888b.  "       << endl;
        cout << "d88P\"88b     \"88b    d88P  d88\"\"88b 888 \"88b "  << endl;
        cout << "888  888 .d888888   d88P   888  888 888  888 "       << endl;
        cout << "Y88b 888 888  888  d88P    Y88..88P 888  888 "       << endl;
        cout << " \"Y88888 \"Y888888 88888888  \"Y88P\"  888  888 "   << endl;
        cout << "     888                                     "       << endl;
        cout << "Y8b d88P   Listening on: 127.0.0.1:" << Config::port << endl;
        cout << " \"Y88P\"    Basedir: " << Config::base_directory       << endl;
        cout << endl;

        cpool.setOnPacket([&](Socket &socket, std::string packet) {
            if (Config::stopRequested)
                throw StopException();

            cout << "[INFO] Received packet `" << packet << "`." << endl;

            tpool.schedule(socket.getFd(), [&, packet](){
                Command *command = nullptr;
                CommandArgsString argsString;

                // In case of a miss, operator[] creates a new instance.
                std::unique_lock<std::mutex> contexts_lock(contexts_mutex);
                Context &context = contexts[socket.getFd()];
                context.fpool = &fpool;
                contexts_lock.unlock();

                try {
                    // Parse the packet and type-check its arguments.
                    std::tie(command, argsString) = commandFromInput(packet);
                    int status;
                    command->middleware(context, &status);
                    CommandArgs args = command->convertAndTypecheckArguments(
                        context, argsString
                    );

                    if (!status) {
                        throw CommandException("access denied.");
                    }

                    command->execute(socket, context, args);
                } catch (const NetworkingException &e) {
                    // NetworkingExceptions should only be logged.
                    cout << "[ERROR] Networking: " << e.what() << endl;
                } catch (const CommandException &e) {
                    // CommandExceptions should be sent back to the client.
                    socket << "Error: " << e.what() << endl;
                } catch (const FilesystemException &e) {
                    // CommandExceptions should be sent back to the client.
                    socket << "Error: " << e.what() << endl;
                }

                delete command;
            });
        });

        cpool.setOnClosing([&](Socket &socket) {
            std::unique_lock<std::mutex> contexts_lock(contexts_mutex);
            if (contexts.count(socket.getFd()) > 0) {
                contexts.at(socket.getFd()).logout();
                contexts.erase(socket.getFd());
            }
        });

        cpool.run();
    } catch (const std::exception& e) {
        cout << "[ERROR] " << e.what() << endl;
    }
}

void stop(int) {
    if (Config::stopRequested) {
        cout << "[INFO] The server will now stop." << endl;
        exit(0);
    } else {
        cout << "[INFO] The server will stop on the next SIGINT (press Ctrl-C again)." << endl;
        Config::stopRequested = true;
    }
}

int main() {
    // The pool of threads in which to allocate the file jobs.
    ThreadPool<FileKeyThread, 8> fpool;

    // The pool of threads in which to allocate the commands.
    ThreadPool<int, 4> tpool;

    // Important! Set the locale of our program to be
    // the same as the one in our environnement.
    std::locale::global(std::locale(""));

    // The entrypoint socket of the server.
    Socket server;

    // The map of session contexts, one per open socket.
    std::unordered_map<int, Context> contexts;
    std::mutex contexts_mutex;

    signal(SIGINT, stop);
    run(server, contexts, contexts_mutex, tpool, fpool);

    // Wait until all the threads are finished executing.
    tpool.join();
    fpool.join();

    // Avoid memory leaks by deallocating all the constructors for
    // the commands registered using the REGISTER_COMMAND macro.
    CommandFactory::destroy();

    return 0;
}

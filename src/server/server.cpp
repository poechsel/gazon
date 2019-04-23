#include <common/grass.h>
#include <common/common.h>
#include <common/config.h>
#include <common/command.h>
#include <common/threadpool.h>
#include <common/filesystem.h>

#include <ctype.h>
#include <iostream>
#include <string>

using std::cout;
using std::endl;

int main() {
    // Important! Set the locale of our program to be
    // the same as the one in our environnement.
    std::locale::global(std::locale(""));
  
    // The entrypoint socket of the server.
    Socket server;

    // The map of session contexts, one per open socket.
    std::unordered_map<int, Context> contexts;

    // The pool of threads in which to allocate the tasks.
    ThreadPool<int> tpool(4);

    try {
        Config::fromFile("grass.conf");
        Filesystem::scan(Config::base_directory);
        Address address = Socket::parseAddress("127.0.0.1", Config::port);
        server.bind(address);
        ConnectionPool cpool = server.listen();

        cpool.setOnConnection([](Socket&) {});

        cpool.setOnIncoming([&](Socket& socket) {
            tpool.schedule(socket.getFd(), [&socket, &contexts](){
                Command *command = nullptr;
                CommandArgsString argsString;

                // In case of a miss, operator[] creates a new instance.
                Context &context = contexts[socket.getFd()];

                try {
                    // Receive the incoming packet, parse and check its arguments.
                    std::string packet;
                    socket >> packet;
                    std::tie(command, argsString) = commandFromInput(packet);
                    CommandArgs args = convertAndTypecheckArguments(
                        context, command->getSpecification(), argsString
                    );

                    command->execute(socket, context, args);
                } catch (std::exception &e) {
                    socket << "Error " << e.what() << endl;
                }

                delete command;
            });
        });

        cpool.setOnClosing([](Socket&) {});

        cpool.run();
    } catch (const ConfigException& e) {
        cout << "[ERROR] Config: " << e.what() << endl;
    } catch (const NetworkingException& e) {
        cout << "[ERROR] Networking: " << e.what() << endl;
    }

    // Wait until all the threads are finished executing.
    tpool.join();

    // Avoid memory leaks by deallocating all the constructors for
    // the commands registered using the REGISTER_COMMAND macro.
    CommandFactory::destroy();

    return 0;
}

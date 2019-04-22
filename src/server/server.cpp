#include <common/grass.h>
#include <ctype.h>
#include <iostream>
#include <string>
#include <common/common.h>
#include <common/config.h>
#include <common/command.h>
#include <common/threadpool.h>
#include <common/filesystem.h>

#include <errno.h>
#include <string.h>

using std::cout;
using std::endl;

int main() {
    try {
        Config::fromFile("grass.conf");
        Filesystem::scan(Config::base_directory);
        Address address = Socket::parseAddress("127.0.0.1", Config::port);
        Socket server;
        server.bind(address);

        ThreadPool<int> tpool(4);
        ConnectionPool cpool = server.listen();

        cpool.setOnConnection([](Socket& from) {
            cout << "onConnection handler" << endl;
        });

        cpool.setOnIncoming([](Socket& from) {
            cout << "onIncoming handler" << endl;

            std::string packet;
            from >> packet;
            cout << "Received: " << packet << endl;

            // TODO: dispatch command in a thread.

            // auto command = CommandFactory::create("hello");
            // command->execute(socket, context, {"www.google.com"});
            // delete command;
        });

        cpool.run();
        tpool.join();
    } catch (const ConfigException& e) {
        cout << "[ERROR] Config: " << e.what() << endl;
    } catch (const NetworkingException& e) {
        cout << "[ERROR] Networking: " << e.what() << endl;
    }

    // Avoids memory leaks by deallocating all the constructors for
    // the commands registered using the REGISTER_COMMAND macro.
    CommandFactory::destroy();
}

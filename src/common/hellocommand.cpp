#include <common/command.h>

class HelloCommand : public Command {
public:
    HelloCommand(): Command(MIDDLEWARE_NONE) {}

    void execute(Socket &socket, Context &context, const CommandArgs &args) {
        std::cout << "Hello on the server [" << args[0].get<std::string>() << "]\n";

        // Send to socket.
        // Make sure to either add std::endl or \n at the end.
        socket << "Hello there!" << std::endl;

        // Receive from socket.
        // This call blocks until it receives a whole line.
        std::string packet;
        socket >> packet;
    }

    Specification getSpecification() const {
        return {ARG_STR};
    }
private:
};

REGISTER_COMMAND(HelloCommand, "hello");

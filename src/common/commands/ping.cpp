#include <common/command.h>

class PingCommand : public Command {
public:
    PingCommand(): Command(MIDDLEWARE_NONE) {}

    void execute(Socket &socket, Context &, const CommandArgs &args) {
        // TODO: add protection
        std::string address = args[0].get<std::string>();
        socket << exec("ping " + address + " -c 1");
    }

    Specification getSpecification() const {
        return {};
    }
private:
};

REGISTER_COMMAND(PingCommand, "ping");

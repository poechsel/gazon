#include <common/command.h>

class PingCommand : public Command {
public:
    PingCommand(): Command(MIDDLEWARE_NONE) {}

    void execute(Socket &socket, Context &, const CommandArgs &args) {
        std::string address = args[0].get<std::string>();
        // The hostname is already escaped since it is of type ARG_HOSTNAME.
        socket << exec("ping '" + address + "' -c 1");
    }

    Specification getSpecification() const {
        return {ARG_HOSTNAME};
    }
private:
};

REGISTER_COMMAND(PingCommand, "ping");

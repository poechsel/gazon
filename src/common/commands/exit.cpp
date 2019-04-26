#include <common/command.h>

class ExitCommand : public Command {
public:
    ExitCommand(): Command(MIDDLEWARE_NONE) {}

    void execute(Socket &socket, Context &context, const CommandArgs &args) {
        socket.close();
    }

    Specification getSpecification() const {
        return {};
    }
private:
};

REGISTER_COMMAND(ExitCommand, "exit");

#include <common/command.h>

class WhoamiCommand : public Command {
public:
    WhoamiCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &socket, Context &context, const CommandArgs &) {
        socket << context.user << std::endl;
    }

    Specification getSpecification() const {
        return {};
    }
private:
};

REGISTER_COMMAND(WhoamiCommand, "whoami");

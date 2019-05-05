#include <common/command.h>

class LogoutCommand : public Command {
public:
    LogoutCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &, Context &context, const CommandArgs &) {
        context.logout();
    }

    Specification getSpecification() const {
        return {};
    }
private:
};

REGISTER_COMMAND(LogoutCommand, "logout");

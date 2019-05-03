#include <common/command.h>

class LoginCommand : public Command {
public:
    LoginCommand(): Command(MIDDLEWARE_NONE) {}

    void execute(Socket &, Context &context, const CommandArgs &args) {
        std::string user = args[0].get<std::string>();
        context.user = user;
        if (Config::userExists(user)) {
            context.isLoggingIn = true;
        } else {
            throw CommandException("Unknown user: " + user);
        }
    }

    Specification getSpecification() const {
        return {ARG_STR};
    }
private:
};

REGISTER_COMMAND(LoginCommand, "login");

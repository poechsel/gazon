#include <common/command.h>

class LoginCommand : public Command {
public:
    LoginCommand(): Command(MIDDLEWARE_LOGGED_OUT) {}

    void execute(Socket &, Context &context, const CommandArgs &args) {
        std::string user = args[0].get<std::string>();
        if (Config::userExists(user)) {
            context.user = user;
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

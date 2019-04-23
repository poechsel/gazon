#include <common/command.h>

class PassCommand : public Command {
public:
    PassCommand(): Command(MIDDLEWARE_LOGGING) {}

    void execute(Socket &, Context &context, const CommandArgs &args) {
        std::string pwd = args[0].get<std::string>();
        if (context.user == "") {
            throw CommandException("Pass must be executed after a call to login");
        } else {
            if (Config::isUserPwdValid(context.user, pwd)) {
                context.isLogged = true;
                context.path = Path("");
            } else {
                context.user = "";
                context.isLogged = false;
                context.path = Path("");
                throw CommandException("Unknown user/pwd pair");
            }
        }
    }

    Specification getSpecification() const {
        return {ARG_STR};
    }
private:
};

REGISTER_COMMAND(PassCommand, "pass");

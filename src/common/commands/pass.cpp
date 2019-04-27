#include <common/command.h>

class PassCommand : public Command {
public:
    PassCommand(): Command(MIDDLEWARE_LOGGING) {}

    void execute(Socket &, Context &context, const CommandArgs &args) {
        /// This will throw a CommandException if the credentials are incorrect.
        context.login(context.user, args[0].get<std::string>());
    }

    Specification getSpecification() const {
        return {ARG_STR};
    }
private:
};

REGISTER_COMMAND(PassCommand, "pass");

#include <common/command.h>

class DateCommand : public Command {
public:
    DateCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &socket, Context &, const CommandArgs &) {
        socket << exec("date");
    }

    Specification getSpecification() const {
        return {};
    }
private:
};

REGISTER_COMMAND(DateCommand, "date");

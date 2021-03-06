#include <common/command.h>
#include <common/filesystem.h>

class RmCommand : public Command {
public:
    RmCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket&, Context&, const CommandArgs &args) {
        Path to_remove = args[0].get<Path>();
        Filesystem::rm(Config::base_directory + to_remove);
    }

    Specification getSpecification() const {
        return {ARG_PATH};
    }
};

REGISTER_COMMAND(RmCommand, "rm");

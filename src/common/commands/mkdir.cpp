#include <common/command.h>
#include <common/filesystem.h>

class MkdirCommand : public Command {
public:
    MkdirCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &, Context &, const CommandArgs &args) {
        Path new_directory = args[0].get<Path>();
        Filesystem::mkdir(new_directory);
    }

    Specification getSpecification() const {
        return {ARG_PATH};
    }
private:
};

REGISTER_COMMAND(MkdirCommand, "mkdir");

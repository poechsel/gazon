#include <common/command.h>
#include <common/filesystem.h>

class RmCommand : public Command {
public:
    RmCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &, Context &context, const CommandArgs &args) {
        Path to_remove = args[0].get<Path>();
        std::cout<<to_remove.string()<<"\n";
        Filesystem::rm(context.getAbsolutePath() + to_remove);
    }

    Specification getSpecification() const {
        return {ARG_PATH};
    }
private:
};

REGISTER_COMMAND(RmCommand, "rm");

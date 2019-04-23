#include <common/command.h>
#include <common/filesystem.h>

class CdCommand : public Command {
public:
    CdCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &, Context &context, const CommandArgs &args) {
        Path relative_path_to_cd = args[0].get<Path>();
        Filesystem::lock();
        DEFER(Filesystem::unlock());

        auto entry = Filesystem::unsafeGetEntryNode(Config::base_directory + relative_path_to_cd);
        if (!entry->isFolder) {
            throw CommandException(relative_path_to_cd.string() + " is not a directory");
        }
        context.relative_path = relative_path_to_cd;
    }

    Specification getSpecification() const {
        return {ARG_PATH};
    }
private:
};

REGISTER_COMMAND(CdCommand, "cd");

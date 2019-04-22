#include <common/command.h>
#include <common/filesystem.h>

class CdCommand : public Command {
public:
    CdCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &, Context &context, const CommandArgs &args) {
        Path path_to_cd = args[0].get<Path>();
        Filesystem::lock();
        DEFER(Filesystem::unlock());

        auto entry = Filesystem::unsafeGetEntryNode(path_to_cd);
        if (!entry->isFolder) {
            throw CommandException(path_to_cd.string() + " is not a directory");
        }
        context.path = path_to_cd;
    }

    Specification getSpecification() const {
        return {ARG_STR};
    }
private:
};

REGISTER_COMMAND(CdCommand, "cd");

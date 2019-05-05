#include <common/command.h>
#include <common/filesystem.h>

class CdCommand : public Command {
public:
    CdCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &socket, Context &context, const CommandArgs &args) {
        Path relative_path_to_cd = args[0].get<Path>();
        Filesystem::lock();
        DEFER(Filesystem::unlock());

        try {
            auto entry = Filesystem::unsafeGetEntryNode(Config::base_directory + relative_path_to_cd);
            if (!entry->isFolder) {
                socket << "cd: not a directory" << relative_path_to_cd.string() << "\n";
            }
            context.relativePath = relative_path_to_cd;
        } catch (const FilesystemException &) {
            socket << "cd: " << relative_path_to_cd.string() << ": No such file or directory\n";
        }
    }

    Specification getSpecification() const {
        return {ARG_PATH};
    }
private:
};

REGISTER_COMMAND(CdCommand, "cd");

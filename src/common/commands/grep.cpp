#include <common/command.h>
#include <common/filesystem.h>
#include <common/regex.h>

class GrepCommand : public Command {
public:
    GrepCommand(): Command(MIDDLEWARE_LOGGED) {}

    void my_grep(const Path &path, Regex &regex, FilesystemEntry *entry, std::vector<std::string> &matched_files) {
        for (auto efile : entry->children) {
            if (Filesystem::isHiddenFile(efile.first)) {
            } else if (efile.second->isFolder) {
                my_grep(path + efile.first, regex, efile.second, matched_files);
            } else {
                Path cpath = path + efile.first;
                File file = Filesystem::unsafeRead(cpath);
                std::string line;
                bool status = true;
                do {
                    status = file.getLine(line);
                    if (regex.match(line)) {
                        matched_files.push_back(cpath.string());
                        break;
                    }
                    line = "";
                } while (status);

                file.close();
            }
        }
    }

    void execute(Socket &socket, Context &context, const CommandArgs &args) {

        std::string pattern = args[0].get<std::string>();
        if (true) {
            Regex regex(".*" + pattern + ".*");

            std::vector<std::string> matched_files;

            {
                Filesystem::lock();
                DEFER(Filesystem::unlock());
                auto entry = Filesystem::unsafeGetEntryNode(context.path);
                my_grep(context.path, regex, entry, matched_files);
            }

            std::sort(matched_files.begin(), matched_files.end(),
                      [](const std::string &a, const std::string &b) {
                          return strcoll(a.c_str(), b.c_str()) < 0;
                      });

            for (auto e : matched_files) {
                socket << e << "\n";
            }
        } else {
            //TODO add sanitization
            socket << exec(std::string("grep -Rl ")
                           + "--exclude-dir=" + Config::temp_directory + " "
                           + pattern + " "
                           + context.path.string()) << "\n";
        }
    }

    Specification getSpecification() const {
        return {ARG_STR};
    }
private:
};

REGISTER_COMMAND(GrepCommand, "grep");

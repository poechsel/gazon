#include <common/command.h>
#include <common/filesystem.h>
#include <common/regex.h>

class GrepCommand : public Command {
public:
    GrepCommand(): Command(MIDDLEWARE_LOGGED) {}

    void my_grep(const Path &path, const Path &relative_path, Regex &regex, FilesystemEntry *entry, std::vector<std::string> &matched_files) {
        for (auto efile : entry->children) {
            if (Filesystem::isHiddenFile(efile.first)) {
            } else if (efile.second->isFolder) {
                my_grep(path + efile.first, relative_path + efile.first, regex, efile.second, matched_files);
            } else {
                Path cpath = path + efile.first;
                File file = Filesystem::unsafeRead(cpath);
                std::string line;
                bool status = true;
                do {
                    status = file.getLine(line);
                    if (regex.match(line)) {
                        matched_files.push_back((relative_path + efile.first).string());
                        break;
                    }
                    line = "";
                } while (status);

                file.close();
            }
        }
    }

    bool heuristic(const Path &absolute_path, std::string regex) {
        Filesystem::lock();
        DEFER(Filesystem::unlock());
        auto entry = Filesystem::unsafeGetEntryNode(absolute_path);
        return (regex.size() <= 8 && entry->nSubFolders == 0 && entry->nRecChildren <= 100 && entry->size <= 100 * 0xffff);
    }

    void execute(Socket &socket, Context &context, const CommandArgs &args) {

        std::string pattern = args[0].get<std::string>();
        if (heuristic(context.getAbsolutePath(), pattern)) {
            Regex regex(".*" + pattern + ".*");

            std::vector<std::string> matched_files;

            {
                Filesystem::lock();
                DEFER(Filesystem::unlock());
                auto entry = Filesystem::unsafeGetEntryNode(context.getAbsolutePath());
                my_grep(context.getAbsolutePath(), Path(""), regex, entry, matched_files);
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
                           + context.getAbsolutePath().string()) << "\n";
        }
    }

    Specification getSpecification() const {
        return {ARG_STR};
    }
private:
};

REGISTER_COMMAND(GrepCommand, "grep");

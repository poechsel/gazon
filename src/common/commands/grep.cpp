#include <common/command.h>
#include <common/filesystem.h>
#include <common/regex.h>

#define GrepCommand_LIMIT_N_FILE 100

class GrepCommand : public Command {
public:
    GrepCommand(): Command(MIDDLEWARE_LOGGED) {}

    void my_grep_aux_unsafe(const Path &path, const Path &relative_path, Regex &regex, FilesystemEntry *entry, std::vector<std::string> &matched_files, short *line_positions) {
        std::vector<std::string> files;
        for (auto efile : entry->children) {
            files.push_back(efile.first);
        }
        std::sort(files.begin(), files.end(),
                  [](const std::string &a, const std::string &b) {
                      return strcoll(a.c_str(), b.c_str()) < 0;
                  });
        for (const std::string &file_name : files) {
            auto node = entry->children[file_name];
            if (Filesystem::isHiddenFile(file_name)) {
            } else if (node->isFolder) {
                my_grep_aux_unsafe(path + file_name, relative_path + file_name, regex, node, matched_files, line_positions);
            } else {
                Path cpath = path + file_name;
                File file = Filesystem::unsafeRead(cpath);
                std::string line;
                int line_number = 0;
                bool status = true;
                do {
                    status = file.getLine(line);
                    if (regex.match(line)) {
                        line_positions[matched_files.size()] = line_number;
                        matched_files.push_back((relative_path + file_name).string());
                        break;
                    }
                    line = "";
                    line_number++;
                } while (status);

                file.close();
            }
        }
    }

    void my_grep_safe(Context &context, Regex &regex, short *line_positions,
                  std::vector<std::string> &matched_files) {
        Filesystem::lock();
        DEFER(Filesystem::unlock());
        auto entry = Filesystem::unsafeGetEntryNode(context.getAbsolutePath());
        my_grep_aux_unsafe(context.getAbsolutePath(), Path(""), regex, entry, matched_files, line_positions);
    }

    void my_grep(Socket &socket, Context &context, Regex &regex) {
        short line_positions[GrepCommand_LIMIT_N_FILE];
        std::vector<std::string> matched_files;

        my_grep_safe(context, regex, line_positions, matched_files);

        for (unsigned int i = 0; i < matched_files.size(); ++i) {
            socket << matched_files[i] << "\n";
        }
    }

    bool heuristic(const Path &absolute_path, std::string regex) {
        Filesystem::lock();
        DEFER(Filesystem::unlock());
        auto entry = Filesystem::unsafeGetEntryNode(absolute_path);
        std::vector<std::string> files;
        for (auto efile : entry->children) {
            files.push_back(efile.first);
        }
        std::sort(files.begin(), files.end(),
                  [](const std::string &a, const std::string &b) {
                      return strcoll(a.c_str(), b.c_str()) < 0;
                  });
        return (regex.size() <= 8 /*&& entry->nSubFolders == 0*/
                && entry->nRecChildren <= GrepCommand_LIMIT_N_FILE
                && entry->size <= GrepCommand_LIMIT_N_FILE * 0xffff);
    }

    void execute(Socket &socket, Context &context, const CommandArgs &args) {
        std::string pattern = args[0].get<std::string>();
        if (heuristic(context.getAbsolutePath(), pattern)) {
            Regex regex(".*" + pattern + ".*");
            my_grep(socket, context, regex);
        } else {
            // The pattern is already escaped since it is of type ARG_PATTERN.
            std::string cmd = "cd " + context.getAbsolutePath().string() +
                "; grep -Rl --exclude-dir=" + Config::temp_directory +
                " -E '" + pattern + "' * | sort";

            socket << exec(cmd) << std::endl;
        }
    }

    Specification getSpecification() const {
        return {ARG_PATTERN};
    }
private:
};

REGISTER_COMMAND(GrepCommand, "grep");

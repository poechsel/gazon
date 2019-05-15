#include <common/command.h>
#include <common/filesystem.h>
#include <common/regex.h>

#define GrepCommand_LIMIT_N_FILE 100

class GrepCommand : public Command {
public:
    GrepCommand(): Command(MIDDLEWARE_LOGGED) {}

    /*
      [path] is the path from the root to the folder we will currently inspect
      [relative_path] is the relative path from the folder we started our exploration
      [regex] is the regex used to match the files
      [entry] is the current node corresponding to [path] in the filesystem
      [matched_files] is a vector of all matched files found so far (in alphabetical order)
      [line_positions] is the array of line position of matches

      Search every subfiles for a pattern and recurse in every subfolder.
      Is not thread safe as we are directly playing with the internals of Filesystem.
    */
    void my_grep_aux_unsafe(const Path &path, const Path &relative_path, Regex &regex, FilesystemEntry *entry, std::vector<std::string> &matched_files, short *line_positions) {
        std::vector<std::string> files;
        // We start by sorting the files in alphabetical order
        for (auto efile : entry->children) {
            files.push_back(efile.first);
        }
        std::sort(files.begin(), files.end(),
                  [](const std::string &a, const std::string &b) {
                      return strcoll(a.c_str(), b.c_str()) < 0;
                  });

        for (const std::string &child_name : files) {
            auto node = entry->children[child_name];
            if (Filesystem::isHiddenFile(child_name)) {
                // Do not test hidden files
                continue;
            } else if (node->isFolder) {
                // Recursively search in sub directories
                my_grep_aux_unsafe(path + child_name, relative_path + child_name, regex, node, matched_files, line_positions);
            } else {
                // Search for a match of `regex` in the file
                Path cpath = path + child_name;
                File file = Filesystem::unsafeRead(cpath);
                std::string line;
                int line_number = 0;
                bool status = true;
                do {
                    status = file.getLine(line);
                    // If we just found a match, add it to the list of matched files,
                    // update line_positions and continue with the next file
                    if (regex.match(line)) {
                        line_positions[matched_files.size()] = line_number;
                        matched_files.push_back((relative_path + child_name).string());
                        break;
                    }
                    line = "";
                    line_number++;
                } while (status);
                file.close();
            }
        }
    }

    // Thread safe wrapper around my_grep_unsafe
    void my_grep_safe(Context &context, Regex &regex, short *line_positions,
                  std::vector<std::string> &matched_files) {
        Filesystem::lock();
        DEFER(Filesystem::unlock());
        auto entry = Filesystem::unsafeGetEntryNode(context.getAbsolutePath());
        my_grep_aux_unsafe(context.getAbsolutePath(), Path(""), regex, entry, matched_files, line_positions);
    }

    void my_grep(Socket &socket, Context &context, Regex &regex) {
        // We are tracking the positions of matches because we are not really sure
        // on how to output grep results...
        short line_positions[GrepCommand_LIMIT_N_FILE];
        // List of all the matched files
        std::vector<std::string> matched_files;

        my_grep_safe(context, regex, line_positions, matched_files);

        // Send the matched files to the socket
        for (unsigned int i = 0; i < matched_files.size(); ++i) {
            socket << matched_files[i] << "\n";
        }
    }

    bool heuristic(const Path &absolute_path, std::string regex) {
        Filesystem::lock();
        DEFER(Filesystem::unlock());
        auto entry = Filesystem::unsafeGetEntryNode(absolute_path);
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
            std::string cmd = "cd " + quote(context.getAbsolutePath().string()) +
                "; grep -Rl --exclude-dir=" + Config::temp_directory +
                " -E " + quote(pattern) + " * | sort";

            socket << exec(cmd) << std::endl;
        }
    }

    Specification getSpecification() const {
        return {ARG_PATTERN};
    }
private:
};

REGISTER_COMMAND(GrepCommand, "grep");

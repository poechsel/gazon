#include <common/grep.h>
#include <common/defer.h>

void Grep::run(Path path, std::string pattern) {
    Regex regex(".*" + pattern + ".*");

    std::vector<std::string> matched_files;

    {
        Filesystem::lock();
        DEFER(Filesystem::unlock());
        auto entry = Filesystem::unsafeGetEntryNode(path);
        for (auto efile : entry->children) {
            if (!efile.second->isFolder
                && !Filesystem::isHiddenFile(efile.first)) {
                File file = Filesystem::unsafeRead(path + efile.first);
                std::string line;
                bool status = true;
                do {
                    status = file.getLine(line);
                    if (regex.match(line)) {
                        matched_files.push_back(efile.first);
                        break;
                    }
                    line = "";
                } while (status);

                file.close();
            }
        }
    }

    std::sort(matched_files.begin(), matched_files.end(),
              [](const std::string &a, const std::string &b) {
                  return strcoll(a.c_str(), b.c_str()) < 0;
              });

    for (auto e : matched_files) {
        std::cout<<e<<"\n";
    }

}

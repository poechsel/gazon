#include <common/grep.h>

void Grep::run(Path path, std::string pattern) {
    auto entry = Filesystem::getEntryNode(path);
    Regex regex(".*" + pattern);

    std::vector<std::string> matched_files;
    for (auto efile : entry->children) {
        if (!efile.second->isFolder
            && !Filesystem::isHiddenFile(efile.first)) {
            std::string cpath = path.string() + "/" + efile.first;
            File file(cpath, "rb");
            std::string line;
            bool status = true;
            do {
                status = file.getLine(line);
                if (regex.asMatch(line)) {
                    matched_files.push_back(efile.first);
                    break;
                }
                line = "";
            } while (status);

            file.close();
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

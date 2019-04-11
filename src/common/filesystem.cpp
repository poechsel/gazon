#include <common/filesystem.h>

FilesystemEntry Filesystem::root;
std::unordered_map<uid_t, std::string> Filesystem::users;
std::unordered_map<uid_t, std::string> Filesystem::groups;

void File::open(const Path &path, std::string type) {
    m_opened_type = type;
    m_file = fopen(path.string().c_str(), type.c_str());
    if (!m_file)
        throw FilesystemException(path.string() + "can't be opened");
}

void File::close() {
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
}

bool File::getLine(std::string &out) {
    char line[64];
    char *result;
    while (true) {
        if ((result = fgets(line, 64, m_file)) != nullptr) {
            auto cpp = std::string(result);
            out += cpp;
            if (cpp[cpp.size() - 1] == '\n') {
                out.pop_back();
                return true;
            }
        } else {
            return false;
        }
    }
    return true;
}

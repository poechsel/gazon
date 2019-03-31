#include <common/filesystem.h>

FilesystemEntry Filesystem::root;
std::unordered_map<uid_t, std::string> Filesystem::users;
std::unordered_map<uid_t, std::string> Filesystem::groups;

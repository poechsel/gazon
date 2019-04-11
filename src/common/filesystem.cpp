#include <common/filesystem.h>

FilesystemEntry Filesystem::root;
std::unordered_map<uid_t, std::string> Filesystem::users;
std::unordered_map<uid_t, std::string> Filesystem::groups;

void Filesystem::scan(const Path &path) {
    if (!path.isAbsolute()) {
        throw FilesystemException("Can only initialize from absolute paths");
    }
    ftw(path.string().c_str(), _callbackftw, 8);
}

void Filesystem::debug(FilesystemEntry *entry, bool showHidden, int level) {
    std::string indent = "";
    for (int i = 0; i < level; ++i)
        indent+="  ";
    for (auto it : entry->children) {
        if (it.first[0] != '.' && !showHidden) {
            std::cout<<indent<<it.first<<"("<<it.second->nRecChildren<<" "<<it.second->size<<")"<<"\n";
            if (it.second->isFolder) {
                debug(it.second, showHidden, level+1);
            }
        }
    }
}

void Filesystem::rm(const Path &path) {
    _removeNodeFromVirtualFS(path);
    nftw(path.string().c_str(), _removewrapper, 8, FTW_DEPTH);
}

void Filesystem::mkdir_(const Path &path) {
    int number_node_to_add = _getNAddedNodes(path);
    if (number_node_to_add != 1) {
        throw FilesystemException("Can't create " + path.string() + ": skipping part of the arborescence");
    }
    if (mkdir(path.string().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        throw FilesystemException(strerror(errno));
    }
    struct stat status;
    if (stat(path.string().c_str(), &status) == -1) {
        throw FilesystemException(strerror(errno));
    }
    _insertNode(path, &status, true);
}

File Filesystem::createFile(const Path &path) {
    int number_node_to_add = _getNAddedNodes(path);
    if (number_node_to_add > 1) {
        throw FilesystemException("Can't create " + path.string() + ": skipping part of the arborescence");
    }
    return File(path, "w");
}

void Filesystem::commit(File &file) {
    file.close();
    struct stat status;
    if (stat(file.path.string().c_str(), &status) == -1) {
        throw FilesystemException(strerror(errno));
    }

    // We try to remove the file from the virtual fs
    // If we can delete it, it means that we are overwritting it
    // Then deleting the virtual binding is a good thing to maintain integrity.
    // Otherwise, an exception will be raised, which we will catch
    try {
        _removeNodeFromVirtualFS(file.path);
    } catch (const std::exception&) {}
    _insertNode(file.path, &status, false);
}

std::string Filesystem::getUser(uid_t uid) {
    if (Filesystem::users.find(uid) == Filesystem::users.end()) {
        struct passwd *entry = getpwuid (uid);
        char const *name = entry ? entry->pw_name : "";
        Filesystem::users[uid] = std::string(name);
    }
    return Filesystem::users[uid];
}

std::string Filesystem::getGroup(uid_t uid) {
    if (Filesystem::groups.find(uid) == Filesystem::groups.end()) {
        auto entry = getgrgid (uid);
        char const *name = entry ? entry->gr_name : "";
        Filesystem::groups[uid] = std::string(name);
    }
    return Filesystem::groups[uid];
}

FilesystemEntry* Filesystem::getEntryNode(const Path &path) {
    FilesystemEntry *entry = &Filesystem::root;
    for (auto el : path) {
        if (entry->children.count(el) > 0) {
            entry = entry->children[el];
        } else {
            throw FilesystemException(path.string() + " not found");
        }
    }
    return entry;
}

bool Filesystem::isHiddenFile(std::string name) {
    return name.size() > 0 ? name[0] == '.' : false;
}

File Filesystem::read(const Path &path) {
    getEntryNode(path); // only to throw an error if we can't access this file
    return File(path, "rb");
}

int Filesystem::_getNAddedNodes(const Path &path) {
    FilesystemEntry* entry = &Filesystem::root;
    auto it = path.begin();
    for (unsigned int i = 0; it != path.end(); it++, i++) {
        if (entry->children.count(*it) == 0) {
            return path.size() - i;
        }
        entry = entry->get(*it);
    }
    return 0;
}
int Filesystem::_insertNode(const Path &path, const struct stat *status, bool isFolder) {
    FilesystemEntry* entry = &Filesystem::root;
    size_t size_file = status->st_size;
    std::string cpath = "";
    auto it = path.begin();
    for (; it + 1 != path.end(); it++) {
        entry = entry->get(*it);
        entry->isFolder = true;
        entry->size += size_file;
        entry->nRecChildren += 1;
        cpath += "/" + *it + "/";
    }
    entry = entry->get(*it);
    entry->status = (struct stat*)malloc(sizeof(struct stat));
    memcpy(entry->status, status, sizeof(struct stat));
    if(isFolder) {
        entry->isFolder = true;
        entry->size = 0;
        entry->nRecChildren = 0;
    } else {
        entry->isFolder = false;
        entry->size = size_file;
        entry->nRecChildren = 0;
    }
    return 0;
}

int Filesystem::_callbackftw(const char *name, const struct stat *status, int type) {
    if(type == FTW_NS)
        return 0;

    std::string s = std::string(name);
    Path path(s);
    return _insertNode(path, status, type == FTW_D);
}

/* remove the metadata corresponding to a path inside the tree
   WARNING: do not physicall delete the file */
void Filesystem::_removeNodeFromVirtualFS(const Path &path) {
    FilesystemEntry* node_to_delete = getEntryNode(path);
    FilesystemEntry *entry = &Filesystem::root;
    for (auto el : path) {
        entry->size -= node_to_delete->size;
        entry->nRecChildren -= node_to_delete->nRecChildren + 1;
        if (entry->children[el] == node_to_delete) {
            entry->children.erase(el);
            break;
        } else {
            entry = entry->children[el];
        }
    }
}

int Filesystem::_removewrapper(const char *path, const struct stat *, int, struct FTW *) {
    return remove(path);
}

#include <common/filesystem.h>

FilesystemEntry Filesystem::m_root;
std::unordered_map<uid_t, std::string> Filesystem::m_users;
std::unordered_map<uid_t, std::string> Filesystem::m_groups;
std::mutex Filesystem::m_mutex;
int Filesystem::m_temp_counter = 0;

void Filesystem::scan(const Path &path) {
    // thread safe
    std::unique_lock<std::mutex> lock(m_mutex);

    if (!path.isAbsolute()) {
        throw FilesystemException("Can only initialize from absolute paths");
    }
    // we are using the unsafe version as the callback will be called very frequently,
    // so we must avoid locking/unlocking to often
    ftw(path.string().c_str(), unsafeCallbackftw, 8);
}

void Filesystem::debug() {
    std::unique_lock<std::mutex> lock(m_mutex);
    unsafeDebug(&m_root);
}
void Filesystem::unsafeDebug(FilesystemEntry *entry, bool showHidden, int level) {
    std::string indent = "";
    for (int i = 0; i < level; ++i)
        indent+="  ";
    for (auto it : entry->children) {
        if (it.first[0] != '.' || showHidden) {
            std::cout<<indent<<it.first
                     <<"("<<it.second->nRecChildren<<" "
                     <<it.second->size<<")"<<"\n";
            if (it.second->isFolder) {
                unsafeDebug(it.second, showHidden, level+1);
            }
        }
    }
}

void Filesystem::rm(const Path &path) {
    // thread safe because _removeNodeFromVirtualFS is threadsafe
    _removeNodeFromVirtualFS(path);
    nftw(path.string().c_str(), _removewrapper, 8, FTW_DEPTH);
}

void Filesystem::mkdir(const Path &path) {
    if (!path.isAbsolute())
        exit(0);
    // Thread safe because _getNumberMissingChild and _insertNode are threadsafe
    int number_node_to_add = _getNumberMissingChild(path);
    if (number_node_to_add == 0) {
        throw FilesystemException("Directory already exists");
    } else if (number_node_to_add > 1) {
        throw FilesystemException("Can't create "
                                  + path.string()
                                  + ": skipping part of the arborescence");
    } else {
        if (::mkdir(path.string().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
            throw FilesystemException(strerror(errno));
        }
        struct stat status;
        if (stat(path.string().c_str(), &status) == -1) {
            throw FilesystemException(strerror(errno));
        }
        _insertNode(path, &status, true);
    }
}

/** Create a TemporaryFile. */
TemporaryFile Filesystem::createFile(const Path &path) {
    // Thread-safe because _getNumberMissingChild is thread-safe.
    int number_node_to_add = _getNumberMissingChild(path);

    // We can either overwrite a file or write a new one.
    if (number_node_to_add > 1) {
        throw FilesystemException(
            "Can't create " + path.string() +
            ": skipping part of the arborescence");
    }

    return TemporaryFile(newTempName(), path);
}

std::string Filesystem::newTempName() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_temp_counter++;
    return Config::temp_directory + std::to_string(m_temp_counter);
}

/** Commit a TemporaryFile to the filesystem. */
void Filesystem::commit(TemporaryFile &file) {
    // Thread-safe because _removeNodeFromVirtualFS and _insertNode are.
    file.commit();

    struct stat status;
    if (stat(file.realPath.string().c_str(), &status) == -1) {
        throw FilesystemException(strerror(errno));
    }

    // We try to remove the file from the virtual filesystem.
    // - If we can delete it, it means that we are overwritting it.
    //   Then deleting the virtual binding is good to maintain integrity.
    // - Otherwise, an exception will be raised, which we will catch.
    try {
        _removeNodeFromVirtualFS(file.realPath);
    } catch (const std::exception&) {}
    _insertNode(file.realPath, &status, false);
}

std::string Filesystem::unsafeGetUser(uid_t uid) {
    if (Filesystem::m_users.find(uid) == Filesystem::m_users.end()) {
        struct passwd *entry = getpwuid (uid);
        char const *name = entry ? entry->pw_name : "";
        Filesystem::m_users[uid] = std::string(name);
    }
    return Filesystem::m_users[uid];
}

std::string Filesystem::unsafeGetGroup(uid_t uid) {
    if (Filesystem::m_groups.find(uid) == Filesystem::m_groups.end()) {
        auto entry = getgrgid (uid);
        char const *name = entry ? entry->gr_name : "";
        Filesystem::m_groups[uid] = std::string(name);
    }
    return Filesystem::m_groups[uid];
}

FilesystemEntry* Filesystem::unsafeGetEntryNode(const Path &path) {
    if (!path.isAbsolute())
        exit(0);
    FilesystemEntry *entry = &Filesystem::m_root;
    for (auto el : path) {
        if (entry->children.count(el) > 0) {
            entry = entry->children[el];
        } else {
            throw FilesystemException(path.string() + ": No such file or directory");
        }
    }
    return entry;
}

bool Filesystem::isHiddenFile(std::string name) {
    return name.size() > 0 ? name[0] == '.' : false;
}

File Filesystem::read(const Path &path) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return unsafeRead(path);
}

File Filesystem::unsafeRead(const Path &path) {
    unsafeGetEntryNode(path); // only to throw an error if we can't access this file
    return File(path);
}

int Filesystem::_getNumberMissingChild(const Path &path) {
    // thread safe
    std::unique_lock<std::mutex> lock(m_mutex);
    FilesystemEntry* entry = &Filesystem::m_root;
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
    std::unique_lock<std::mutex> lock(m_mutex);
    return unsafeInsertNode(path, status, isFolder);
}

int Filesystem::unsafeInsertNode(const Path &path, const struct stat *status, bool isFolder) {
    FilesystemEntry* entry = &Filesystem::m_root;
    size_t size_file = status->st_size;
    std::string cpath = "";
    auto it = path.begin();
    for (; it + 1 != path.end(); it++) {
        entry = entry->get(*it);
        entry->isFolder = true;
        entry->size += size_file;
        entry->nRecChildren += 1;
        entry->nSubFolders += isFolder;
        cpath += "/" + *it + "/";
    }
    entry = entry->get(*it);
    entry->status = (struct stat*)malloc(sizeof(struct stat));
    memcpy(entry->status, status, sizeof(struct stat));
    if(isFolder) {
        entry->isFolder = true;
        entry->size = 0;
        entry->nRecChildren = 0;
        entry->nSubFolders = 0;
    } else {
        entry->isFolder = false;
        entry->size = size_file;
        entry->nRecChildren = 0;
        entry->nSubFolders = 0;
    }
    return 0;
}

int Filesystem::unsafeCallbackftw(const char *name, const struct stat *status, int type) {
    if(type == FTW_NS)
        return 0;

    std::string s = std::string(name);
    Path path(s);
    // do not add our temp file
    if (path.string() == Config::temp_directory)
        return 0;
    return unsafeInsertNode(path, status, type == FTW_D);
}

/* remove the metadata corresponding to a path inside the tree
   WARNING: do not physicall delete the file */
void Filesystem::_removeNodeFromVirtualFS(const Path &path) {
    // thread safe
    std::unique_lock<std::mutex> lock(m_mutex);
    FilesystemEntry* node_to_delete = unsafeGetEntryNode(path);
    FilesystemEntry *entry = &Filesystem::m_root;
    for (auto el : path) {
        entry->size -= node_to_delete->size;
        entry->nRecChildren -= node_to_delete->nRecChildren + 1;
        entry->nSubFolders -= node_to_delete->nSubFolders + node_to_delete->isFolder;
        if (entry->children[el] == node_to_delete) {
            entry->nRecChildren--;
            entry->children.erase(el);
            break;
        } else {
            entry = entry->children[el];
        }
    }
}

int Filesystem::_removewrapper(const char *path, const struct stat *, int, struct FTW *) {
    // does not need to be thread safe
    return remove(path);
}

void Filesystem::lock() {
    m_mutex.lock();
}

void Filesystem::unlock() {
    m_mutex.unlock();
}

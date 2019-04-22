#pragma once

#include <common/common.h>

#include <mutex>

#include <dirent.h>
#include <ftw.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/time.h>
#include <cstring>
#include <iostream>

#include <common/path.h>
#include <common/file.h>

/* An entry of the filesystem.
   Completely thread-unsafe.
   You must ensure that no concurrent read or writes on this happens.
   If you're getting a FilesystemEntry from `Filesystem`, you should call
   `Filesystem::lock` and `Filesystem::unlock`
*/
class FilesystemEntry {
public:
    FilesystemEntry(): size(0), nRecChildren(0), isFolder(false), status(nullptr) {}
    ~FilesystemEntry() {
        if (status != nullptr)
            free(status);
        for (auto it : children)
            delete it.second;
    }
    // If children is empty this represents a file
    // Otherwise a folder
    std::unordered_map<std::string, FilesystemEntry*> children;
    // For files: size of the file on the disk
    // For folders: size of the folder (sum of all size of all chidren)
    size_t size;

    // Count of all the children recursively
    size_t nRecChildren;

    bool isFolder;

    FilesystemEntry* get(std::string name) {
        if (children.find(name) == children.end()) {
            children[name] = new FilesystemEntry;
        }
        return children[name];
    }

    struct stat *status;
};

/* Unless prefixes by "unsafe", this functions are
   thread safe */
class Filesystem {
public:
    /* Scan all folders with parent path and add them to our local database */
    static void scan(const Path &path);

    static void debug();
    static void unsafeDebug(FilesystemEntry *entry, bool showHidden = false, int level=0);

    /* Remove a path or directory */
    static void rm(const Path &path);

    /* Create a directory */
    static void mkdir(const Path &path);

    /* Create a file.
       Does not guarantee that until a matching call to [commit] the created
       file is physically store or even stored at the location pointed by [path] */
    static ProxyWriteFile createFile(const Path &path);

    /* Committing a file to the filesystem means that:
       - we close the file [file]
       - the file is present on the physicall file system
       - the file is added to the virtual filesystem
    */
    static void commit(ProxyWriteFile &file);

    /* Get the unix group corresponding to uid [uid].
       Do some caching */
    static std::string unsafeGetUser(uid_t uid);

    /* Get the unix group corresponding to uid [uid].
       Do some caching */
    static std::string unsafeGetGroup(uid_t uid);

    /* Get the filesystem node corresponding to path [path].
       No "safe" variant are implemented as the returned FilesystemEntry is
       not thread safe.
     */
    static FilesystemEntry* unsafeGetEntryNode(const Path &path);

    // do not need to be thread safe
    // Primitive to know if a file is Hidden
    static bool isHiddenFile(std::string name);

    /* Return an handler to the file pointed at path. Can
       raise an error */
    static File read(const Path &path);
    static File unsafeRead(const Path &path);

    /* Primitive to lock and unlock this filesystem
       Must be called when explicitely calling unsafe primitives in a
       multithreaded context.
       Ex: see implementation of grep & ls */
    static void lock();

    static void unlock();

private:
    /* Count the number of nodes needed to be added in order for path to be valid
       Ex: the fs contains foo/bar, we want to create foo/bar/baz/fox
       -> we must create 2 child */
    static int _getNumberMissingChild(const Path &path);

    /* Insert a node inside the filesystem and make sure that the property of the various
       fileentry (ex: size, nRecChildren) are updated */
    static int _insertNode(const Path &path, const struct stat *status, bool isFolder);
    static int unsafeInsertNode(const Path &path, const struct stat *status, bool isFolder);

    /* callback for [ftw]: used to scan the filesystem */
    static int unsafeCallbackftw(const char *name, const struct stat *status, int type);

    /* remove the metadata corresponding to a path inside the tree
       WARNING: do not physicall delete the file */
    static void _removeNodeFromVirtualFS(const Path &path);

    /* wrapper around the unix [remove] function */
    static int _removewrapper(const char *path, const struct stat *, int, struct FTW *);

    static std::string newTempName();

    static std::mutex m_mutex;
    static FilesystemEntry m_root;
    static std::unordered_map<uid_t, std::string> m_users;
    static std::unordered_map<uid_t, std::string> m_groups;
    static int m_temp_counter;
    static std::string m_temp_root;
};

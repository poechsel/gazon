#pragma once

#include <common/common.h>
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

class Filesystem {
public:
    static FilesystemEntry root;
    static std::unordered_map<uid_t, std::string> users;
    static std::unordered_map<uid_t, std::string> groups;
    /* Scan all folders with parent path and add them to our local database */
    static void scan(const Path &path);

    static void debug(FilesystemEntry *entry, bool showHidden = false, int level=0);

    static void rm(const Path &path);

    static void mkdir_(const Path &path);

    static File createFile(const Path &path);

    static void commit(File &file);

    static std::string getUser(uid_t uid);

    static std::string getGroup(uid_t uid);

    static FilesystemEntry* getEntryNode(const Path &path);

    static bool isHiddenFile(std::string name);

    static File read(const Path &path);

private:
    static int _getNAddedNodes(const Path &path);
    static int _insertNode(const Path &path, const struct stat *status, bool isFolder);
    static int _callbackftw(const char *name, const struct stat *status, int type);

    /* remove the metadata corresponding to a path inside the tree
       WARNING: do not physicall delete the file */
    static void _removeNodeFromVirtualFS(const Path &path);
    static int _removewrapper(const char *path, const struct stat *, int, struct FTW *);
};

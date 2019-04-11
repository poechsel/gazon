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

class Path {
public:
    typedef std::vector<std::string>::const_iterator iterator;

    iterator begin() const {
        return m_parts.begin();
    }
    iterator end() const {
        return m_parts.end();
    }

    unsigned int size() const {
        return m_parts.size();
    }

    Path(std::string const &path_str):
        m_string(path_str)
    {
        if (path_str[0] == '/') {
            m_is_absolute = true;
        } else {
            m_is_absolute = false;
        }
        auto parts = splitString(path_str, '/');
        for (const std::string cpart : parts) {
            if (cpart == ".") {
                continue;
            } else if (cpart == ".." && m_parts.size() > 0) {
                m_parts.pop_back();
            } else if (cpart == "~") {
                m_parts.push_back("home");
                auto current_user = getHome();
                m_parts.push_back(current_user);
            } else {
                m_parts.push_back(cpart);
            }
        }
    }

    std::string getHome() const {
        std::string current_usera;
        getlogin_r((char*)current_usera.c_str(), 32);
        return current_usera;
    }
    bool isAbsolute() const {
        return m_is_absolute;
    }

    std::string string() const {
        return m_string;
    }

    Path& operator+=(const std::string &next_part) {
        m_parts.push_back(next_part);
        m_string += "/" + next_part;
        return *this;
    }
 
    friend Path operator+(Path path, const std::string &next_part) {
        path += next_part;
        return path;
    }

private:
    std::vector<std::string> m_parts;
    bool m_is_absolute;
    std::string m_string;
};

struct FilesystemException : public std::runtime_error {
public:
    FilesystemException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

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

class File {
public:
    File(const Path &path, std::string type): path(path), m_file(nullptr) {
        open(path, type);
    }
    ~File() {
        close();
    }
    void open(const Path &path, std::string type);
    void close();
    bool getLine(std::string &out);
    Path path;
private:
    FILE* m_file;
    std::string m_opened_type;
};

class Filesystem {
public:
    /* Scan all folders with parent path and add them to our local database */
    static void scan(const Path &path) {
        if (!path.isAbsolute()) {
            throw FilesystemException("Can only initialize from absolute paths");
        }
        ftw(path.string().c_str(), _callbackftw, 8);
    }
    static FilesystemEntry root;
    static std::unordered_map<uid_t, std::string> users;
    static std::unordered_map<uid_t, std::string> groups;

    static void debug(FilesystemEntry *entry, bool showHidden = false, int level=0) {
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

    static void rm(const Path &path) {
        _removeNode(path);
        nftw(path.string().c_str(), _removewrapper, 8, FTW_DEPTH);
    }

    static void mkdir_(const Path &path) {
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

    static File createFile(const Path &path) {
        int number_node_to_add = _getNAddedNodes(path);
        if (number_node_to_add != 1) {
            throw FilesystemException("Can't create " + path.string() + ": skipping part of the arborescence");
        }
        return File(path, "w");
    }

    static void commit(File &file) {
        file.close();
        struct stat status;
        if (stat(file.path.string().c_str(), &status) == -1) {
            throw FilesystemException(strerror(errno));
        }
        _insertNode(file.path, &status, false);
    }

    static std::string getUser(uid_t uid) {
        if (Filesystem::users.find(uid) == Filesystem::users.end()) {
            struct passwd *entry = getpwuid (uid);
            char const *name = entry ? entry->pw_name : "";
            Filesystem::users[uid] = std::string(name);
        }
        return Filesystem::users[uid];
    }

    static std::string getGroup(uid_t uid) {
        if (Filesystem::groups.find(uid) == Filesystem::groups.end()) {
            auto entry = getgrgid (uid);
            char const *name = entry ? entry->gr_name : "";
            Filesystem::groups[uid] = std::string(name);
        }
        return Filesystem::groups[uid];
    }

    static FilesystemEntry* getEntryNode(const Path &path) {
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

    static bool isHiddenFile(std::string name) {
        return name.size() > 0 ? name[0] == '.' : false;
    }

    static File read(const Path &path) {
        getEntryNode(path); // only to throw an error if we can't access this file
        return File(path, "rb");
    }

private:
    static int _getNAddedNodes(const Path &path) {
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
   static int _insertNode(const Path &path, const struct stat *status, bool isFolder) {
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

    static int _callbackftw(const char *name, const struct stat *status, int type) {
        if(type == FTW_NS)
            return 0;

        std::string s = std::string(name);
        Path path(s);
        return _insertNode(path, status, type == FTW_D);
    }

    /* remove the metadata corresponding to a path inside the tree
       WARNING: do not physicall delete the file */
    static void _removeNode(const Path &path) {
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

    static int _removewrapper(const char *path, const struct stat *, int, struct FTW *) {
        return remove(path);
    }

};

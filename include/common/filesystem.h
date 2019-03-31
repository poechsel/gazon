#pragma once

#include <common/common.h>
#include <dirent.h>
#include <ftw.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unordered_map>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/time.h>

char ftypelet (mode_t bits)
{
    if (S_ISREG (bits))
        return '-';
    if (S_ISDIR (bits))
        return 'd';

    return '?';
}

void
strmode (mode_t mode, char *str)
{
    str[0] = ftypelet (mode);
    str[1] = mode & S_IRUSR ? 'r' : '-';
    str[2] = mode & S_IWUSR ? 'w' : '-';
    str[3] = (mode & S_ISUID
              ? (mode & S_IXUSR ? 's' : 'S')
              : (mode & S_IXUSR ? 'x' : '-'));
    str[4] = mode & S_IRGRP ? 'r' : '-';
    str[5] = mode & S_IWGRP ? 'w' : '-';
    str[6] = (mode & S_ISGID
              ? (mode & S_IXGRP ? 's' : 'S')
              : (mode & S_IXGRP ? 'x' : '-'));
    str[7] = mode & S_IROTH ? 'r' : '-';
    str[8] = mode & S_IWOTH ? 'w' : '-';
    str[9] = (mode & S_ISVTX
              ? (mode & S_IXOTH ? 't' : 'T')
              : (mode & S_IXOTH ? 'x' : '-'));
    str[10] = ' ';
    str[11] = '\0';
}

std::vector<std::string> splitString(const std::string &s, char sep) {
    std::vector<std::string> out;
    std::string c = "";
    for (const char x : s) {
        if (x == sep) {
            if (c != "")
                out.push_back(c);
            c = "";
        } else {
            c += x;
        }
    }
    if (c != "")
        out.push_back(c);
    return out;
}

class Path {
public:
    typedef std::vector<std::string>::iterator iterator;

    iterator begin() {
        return m_parts.begin();
    }
    iterator end() {
        return m_parts.end();
    }

    unsigned int size() {
        return m_parts.size();
    }

    Path(std::string const &path_str) {
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

private:
    std::vector<std::string> m_parts;
    bool m_is_absolute;
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

int timespec_cmp (struct timespec a, struct timespec b)
{
    if (a.tv_sec < b.tv_sec)
        return -1;
    if (a.tv_sec > b.tv_sec)
        return 1;

    return a.tv_nsec - b.tv_nsec;
}

void gettime (struct timespec *ts)
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
}

class Filesystem {
public:
    Filesystem(std::string root):
        m_root(root)
    {
    }


    /* Scan all folders with parent root/ to rebuild our
       virtual filesystem on restart */
    void scanAll() {
        ftw(m_root.c_str(), callbackftw, 8);
    }
    static FilesystemEntry root;
    static std::unordered_map<uid_t, std::string> users;
    static std::unordered_map<uid_t, std::string> groups;

    void debug(FilesystemEntry *entry, int level=0) {
        std::string indent = "";
        for (int i = 0; i < level; ++i)
            indent+="  ";
        for (auto it : entry->children) {
            if (it.second->isFolder) {
                debug(it.second, level+1);
            }
        }
    }

    static int callbackftw(const char *name, const struct stat *status, int type) {
        if(type == FTW_NS)
            return 0;

        std::string s = std::string(name);
        Path path(s);
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
        if(type == FTW_F) {
            entry->isFolder = false;
            entry->size = size_file;
            entry->nRecChildren = 0;
        } else if(type == FTW_D) {
            entry->isFolder = true;
            entry->size = 0;
            entry->nRecChildren = 0;
        }
        return 0;
    }

    static std::string getUser(uid_t uid) {
        if (Filesystem::users.find(uid) == Filesystem::users.end()) {
            struct passwd *entry = getpwuid (uid);
            char const *name = entry ? entry->pw_name : "";
            Filesystem::users[uid] = std::string(name);
        }
        return Filesystem::users[uid];
    }
    /* TODO: add a cache*/
    static std::string getGroup(uid_t uid) {
        if (Filesystem::groups.find(uid) == Filesystem::groups.end()) {
            auto entry = getgrgid (uid);
            char const *name = entry ? entry->gr_name : "";
            Filesystem::groups[uid] = std::string(name);
        }
        return Filesystem::groups[uid];
    }

    static FilesystemEntry* getEntryNode(Path path) {
        FilesystemEntry *entry = &Filesystem::root;
        for (auto el : path) {
            entry = entry->children[el];
        }
        return entry;
    }

    static bool isHiddenFile(std::string name) {
        return name.size() > 0 ? name[0] == '.' : false;
    }

    static bool needsQuoting(std::string name) {
        for (auto c : name) {
            if (c <= 0x20 || c >= 0x7f)
                return true;
        }
        return false;
    }



    static void ls(Path path) {
        FilesystemEntry *entry =
            Filesystem::root.children["home"]
            ->children["pierre"];
        entry = Filesystem::getEntryNode(path);
        struct lsdata {
            std::string mode;
            std::string date;
            std::string group;
            std::string user;
            std::string name;
            std::string nlink;
            std::string size;
        };
        std::vector<lsdata> files;
        int mode_length = 0;
        int date_length = 0;
        int group_length = 0;
        int user_length = 0;
        int name_length = 0;
        int nlink_length = 0;
        int size_length = 0;

        int nblocks = 0;

        bool quotingExists = false;

        for (auto element : entry->children) {
            if (!isHiddenFile(element.first) && element.second->status) {
                struct stat* status = element.second->status;
                nblocks += status->st_blocks / 2;

                struct timespec when_timespec;
                when_timespec.tv_sec = status->st_mtime;
                when_timespec.tv_nsec = 0;
                struct tm *when_local = localtime(&status->st_mtime);
                struct timespec current_time;
                gettime(&current_time);

                struct timespec six_months_ago;

                six_months_ago.tv_sec = current_time.tv_sec - 31556952 / 2;
                six_months_ago.tv_nsec = current_time.tv_nsec;

                bool recent = (timespec_cmp (six_months_ago, when_timespec) < 0
                               && (timespec_cmp (when_timespec, current_time) < 0));
                char buf[100];
                strftime(buf, 100, m_date_fmt[12 * recent + when_local->tm_mon].c_str(), when_local);

                char str[11];
                strmode(status->st_mode, str);

                quotingExists |= needsQuoting(element.first);

                lsdata filedata = {
                                   std::string(str),
                                   std::string(buf),
                                   getUser(status->st_uid),
                                   getGroup(status->st_uid),
                                   element.first,
                                   std::to_string(status->st_nlink),
                                   std::to_string(status->st_size)
                };

                mode_length = std::max(mode_length, (int)filedata.mode.size());
                date_length = std::max(date_length, (int)filedata.date.size());
                group_length = std::max(group_length, (int)filedata.group.size());
                user_length = std::max(user_length, (int)filedata.user.size());
                name_length = std::max(name_length, (int)filedata.name.size());
                nlink_length = std::max(nlink_length, (int)filedata.nlink.size());
                size_length = std::max(size_length, (int)filedata.size.size());
                files.push_back(filedata);
            }
        }

        std::sort(files.begin(), files.end(),
                  [](const lsdata &a, const lsdata &b) {
                      return strcoll(a.name.c_str(), b.name.c_str()) < 0;
                  });

        char format_str_quote_with[128];
        char format_str_quote_without[128];
        char format_str_no_quote[128];
        snprintf(format_str_quote_with, 128,
                 "%%s%%%ds %%-%ds %%-%ds %%%ds %%s '%%s'\n",
                 nlink_length,
                 user_length,
                 group_length,
                 size_length
                 );
        snprintf(format_str_quote_without, 128,
                 "%%s%%%ds %%-%ds %%-%ds %%%ds %%s  %%s\n",
                 nlink_length,
                 user_length,
                 group_length,
                 size_length
                 );
        snprintf(format_str_no_quote, 128,
                 "%%s%%%ds %%-%ds %%-%ds %%%ds %%s %%s\n",
                 nlink_length,
                 user_length,
                 group_length,
                 size_length
                 );
        char c_str[1024];
        printf("total %d\n", nblocks);
        for (auto lsdata : files) {
            std::string name = lsdata.name;
            char *format_str = format_str_no_quote;
            if (quotingExists) {
                if (Filesystem::needsQuoting(name)) {
                    format_str = format_str_quote_with;
                } else {
                    format_str = format_str_quote_without;
                }
            } 
            snprintf(c_str, 1024,
                     format_str,
                     lsdata.mode.c_str(),
                     lsdata.nlink.c_str(),
                     lsdata.user.c_str(),
                     lsdata.group.c_str(),
                     lsdata.size.c_str(),
                     lsdata.date.c_str(),
                     lsdata.name.c_str()
                     );
            printf(c_str);
        }
    }
private:
    static std::array<std::string, 24> m_date_fmt;
    std::string m_root;
};

FilesystemEntry Filesystem::root;
std::unordered_map<uid_t, std::string> Filesystem::users;
std::unordered_map<uid_t, std::string> Filesystem::groups;

std::array<std::string, 12 * 2> Filesystem::m_date_fmt = {
                                                          "Jan %e  %Y",
                                                          "Feb %e  %Y",
                                                          "Mar %e  %Y",
                                                          "Apr %e  %Y",
                                                          "May %e  %Y",
                                                          "Jun %e  %Y",
                                                          "Jul %e  %Y",
                                                          "Aug %e  %Y",
                                                          "Sep %e  %Y",
                                                          "Oct %e  %Y",
                                                          "Nov %e  %Y",
                                                          "Dec %e  %Y",
                                                          "Jan %e %H:%M",
                                                          "Feb %e %H:%M",
                                                          "Mar %e %H:%M",
                                                          "Apr %e %H:%M",
                                                          "May %e %H:%M",
                                                          "Jun %e %H:%M",
                                                          "Jul %e %H:%M",
                                                          "Aug %e %H:%M",
                                                          "Sep %e %H:%M",
                                                          "Oct %e %H:%M",
                                                          "Nov %e %H:%M",
                                                          "Dec %e %H:%M"
};

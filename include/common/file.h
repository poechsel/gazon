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

struct FilesystemException : public std::runtime_error {
public:
    FilesystemException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
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
protected:
    FILE* m_file;
    std::string m_opened_type;
};

class ProxyWriteFile {
public:
    ProxyWriteFile(const std::string tempPath, const Path &realPath)
        : tempPath(tempPath), path(realPath) {
        m_file = fopen(tempPath.c_str(), "w");
        if (!m_file)
            throw FilesystemException(tempPath + "can't be opened");
    }
    ~ProxyWriteFile() {
        close();
    }
    void open(const Path &path, std::string type);
    void close();
    void write(uint32_t n, const char *content);
    std::string tempPath;
    Path path;
private:
    FILE* m_file;
};

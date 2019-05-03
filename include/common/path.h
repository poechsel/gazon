#pragma once

#include <common/common.h>
#include <stdio.h>
#include <unordered_map>
#include <vector>

class Context;

class Path {
public:
    typedef std::vector<std::string>::const_iterator iterator;

    iterator begin() const {
        return m_parts.begin();
    }
    iterator end() const {
        return m_parts.end();
    }

    unsigned int size() const;

    Path(std::string const &path_str, const Context *context = nullptr);

    bool isAbsolute() const;

    // return true if this path attempts to traverse a parent directory
    // (for exemple foo/../../foo)
    bool attemptParentTraversal() const;

    std::string string() const;

    size_t length() const;

    Path& operator+=(const std::string &next_part);
    friend Path operator+(Path path, const std::string &next_part) {
        path += next_part;
        return path;
    }

    Path& operator+=(const Path &other);
    friend Path operator+(Path path, const Path &other) {
        path += other;
        return path;
    }

    void setRelative();
    Path() {}
private:
    std::vector<std::string> m_parts;
    bool m_is_absolute;
    std::string m_string;

    void reconstructStringView();
    bool addPathPart(const std::string &next_part);
};



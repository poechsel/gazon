#pragma once

#include <common/common.h>
#include <stdio.h>
#include <unordered_map>
#include <vector>

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

    Path(std::string const &path_str)
    {
        if (path_str[0] == '/') {
            m_is_absolute = true;
        } else {
            m_is_absolute = false;
        }
        m_parts.clear();
        auto parts = splitString(path_str, '/');
        for (const std::string cpart : parts) {
            if (cpart == "." || cpart == "") {
                continue;
            } else if (cpart == ".."
                       && m_parts.size() > 0
                       && m_parts[m_parts.size() - 1] != "..") {
                m_parts.pop_back();
            } else if (cpart == "~") {
                m_parts.push_back("home");
                auto current_user = getHome();
                m_parts.push_back(current_user);
            } else {
                m_parts.push_back(cpart);
            }
        }
        reconstructStringView();
    }

    std::string getHome() const {
        std::string current_usera;
        // TODO
        throw NetworkingException("'~' expansion not implemented");
        //getlogin_r((char*)current_usera.c_str(), 32);
        return current_usera;
    }
    bool isAbsolute() const {
        return m_is_absolute;
    }

    // return true if this path attempts to traverse a parent directory
    // (for exemple foo/../../foo)
    bool attemptParentTraversal() const {
        int depth = 0;
        for (auto part : m_parts) {
            if (part == "..")
                depth --;
            else
                depth ++;
            if (depth < 0)
                return true;
        }
        return false;
    }

    std::string string() const {
        return m_string;
    }

    size_t length() const {
        return m_string.size();
    }

    Path& operator+=(const std::string &next_part) {
        if (addPathPart(next_part)) {
            reconstructStringView();
        }
        return *this;
    }

    friend Path operator+(Path path, const std::string &next_part) {
        path += next_part;
        return path;
    }

    Path& operator+=(const Path &other) {
        if (other.isAbsolute()) {
            m_parts = other.m_parts;
            m_string = other.m_string;
        } else {
            bool needReconstruction = false;
            for (const auto part : other.m_parts)
                needReconstruction |= addPathPart(part);
            if (needReconstruction)
                reconstructStringView();
        }
        return *this;
    }

    friend Path operator+(Path path, const Path &other) {
        path += other;
        return path;
    }

    void setRelative() {
        m_is_absolute = false;
    }

private:
    std::vector<std::string> m_parts;
    bool m_is_absolute;
    std::string m_string;

    void reconstructStringView() {
        m_string = "";
        for (unsigned int i = 0; i < m_parts.size(); ++i) {
            m_string += ((i > 0 || m_is_absolute) ? "/" : "") + m_parts[i];
        }
    }

    bool addPathPart(const std::string &next_part) {
        if (next_part == ".."
            && m_parts.size() > 0
            && m_parts[m_parts.size() - 1] != "..") {
            m_parts.pop_back();
            return true;
        } else if (next_part == ".") {
            return false;
        } else {
            m_parts.push_back(next_part);
            return true;
        }
    }
};



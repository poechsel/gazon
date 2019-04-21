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
        // TODO
        throw NetworkingException("'~' expansion not implemented");
        //getlogin_r((char*)current_usera.c_str(), 32);
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

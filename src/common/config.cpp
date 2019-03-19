#include <common/config.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>


ConfigException::ConfigException(std::string path, int line, std::string m):
    m_path(path), m_line(line), m_message(m) {
    std::string error;
    if (m_line <= 0) {
        error = m_message;
    } else {
        error = m_path + ":" + std::to_string(m_line) + ", " + m_message;
    }
    m_buffer = new char[error.size()];
    strncpy(m_buffer, error.c_str(), error.size());
}

ConfigException::~ConfigException() {
    delete m_buffer;
}

const char* ConfigException::what() const throw(){
    return m_buffer;
}

void parseLine(std::string const &path, std::istringstream &iss, int nline, Config &config) {
    std::string keyword;

    if (iss >> keyword) {
        if (keyword == "base") {
            std::string base_directory;
            if (iss >> base_directory) {
                config.base_directory = base_directory;
            } else {
                throw ConfigException(path, nline, "`base` keyword expects one string");
            }
        } else if (keyword == "port") {
            uint16_t port;
            if (iss >> port) {
                config.port = port;
            } else {
                throw ConfigException(path, nline, "`port` keyword excepts one int");
            }
        } else if (keyword == "user") {
            std::string user, pwd;
            if (iss >> user >> pwd) {
                config.users_pwd.push_back({user, pwd});
            } else {
                throw ConfigException(path, nline, "`user` keyword excepts two strings");
            }
        } else if (keyword.size() >= 1 && keyword[0] == '#') {
            // do nothing, this is a comment
        } else {
            throw ConfigException(path, nline, "Can't understand keyword `" + keyword + "`");
        }
    }
}

Config Config::fromFile(std::string path) {
    std::ifstream file;
    file.open(path);

    if (!file) {
        throw ConfigException(path, 0, "file " + path + " not found");
    } else {
        Config config;
        std::string line;
        int nline = 0;
        while (std::getline(file, line)) {
            nline++;
            std::istringstream iss(line);
            parseLine(path, iss, nline, config);
        }
        return config;
    }
}

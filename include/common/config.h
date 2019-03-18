#pragma once

#include <vector>
#include <string>
#include <exception>
#include <stdexcept>


struct ConfigException : public std::exception {
public:
    ConfigException(std::string path, int line, std::string m);

    ~ConfigException();

	const char* what () const throw ();
private:
    std::string m_path;
    int m_line;
    std::string m_message;
    char *m_buffer;
};

class Config {
public:
    static Config fromFile(std::string path);
    uint16_t port;
    std::string base_directory;
    std::vector<std::pair<std::string, std::string>> users_pwd;
};

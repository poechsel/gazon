#pragma once

#include <vector>
#include <string>
#include <exception>
#include <stdexcept>
#include <unordered_map>


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
    static void fromFile(std::string path);
    static uint16_t port;
    static std::string base_directory;

    static bool userExists(const std::string user);
    static bool isUserPwdValid(const std::string user, const std::string pwd);
    static void setUserPwd(const std::string user, const std::string pwd);
protected:
    static std::unordered_map<std::string, std::string> m_users_pwd;
};

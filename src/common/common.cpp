#include <common/common.h>
#include <errno.h>
#include <regex>

/** Return a formatted string containing the current value of errno. */
std::string formatError(const std::string& message) {
    std::stringstream error;
    error << message << ": " << ::strerror(errno) << ".";
    return error.str();
}

/** Quote a string to safely insert it inside a shell command. */
std::string quote(const std::string &s) {
    return "'" + std::regex_replace(s, std::regex("'"), "'\"'\"'") + "'";
}

/** Enforce that the result of a function to be >= 0. */
void enforce(int i) {
    if (i < 0) {
        throw CommandException(formatError("failed"));
    }
}

/** Return the index of the first non empty char in `s` after `start`. */
uint skipEmptyChars(uint start, std::string const& s) {
    uint end = start;
    for (uint i = start; i < s.size() && std::isspace(s[i]); ++i) {
        end = i + 1;
    }
    return end;
}

/** Return the index of the first empty char in `s` after `start`. */
uint skipNonEmptyChars(uint start, std::string const& s) {
    uint end = start;
    for (uint i = start; i < s.size() && !std::isspace(s[i]); ++i) {
        end = i + 1;
    }
    return end;
}

/** Return the index of the first char in `prohibited`. */
uint skipUntil(uint start, std::string const &s, std::string const &chars) {
    uint end = start;
    for (uint i = start; i < s.size() && chars.find(s[i]) == std::string::npos; ++i) {
        end = i + 1;
    }
    return end;
}

/** Return the position of the end of the argument. */
std::tuple<uint, bool> skipArg(uint start, std::string const &s) {
    uint end = start;
    if (s[start] == '\'') {
        end = start + 1;
        for (uint i = start + 1; i < s.size() && s[i] != '\''; ++i) {
            end = i + 1;
        }
        return std::make_tuple(end+1, true);
    } else if (s[start] == '"') {
        end = start + 1;
        for (uint i = start + 1; i < s.size() && !(s[i] == '"' && s[i-1] != '\\'); ++i) {
            end = i + 1;
        }
        return std::make_tuple(end+1, true);
    } else {
        end = skipNonEmptyChars(start, s);
    }
    return std::make_tuple(end, false);
}

/** Split a string into a vector of strings, using a given `sep`. */
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


/** Execute a shell command using popen. */
std::string exec(std::string cmd) {
    std::array<char, 256> buffer;
    std::string result = "";
    std::string command = cmd + " 2>&1";
    std::cout << "[INFO] Executing `" << command << "`." << std::endl;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    try {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
    } catch (const std::exception& e) {
        pclose(pipe);
        result = "";
        throw CommandException(e.what());
    }
    pclose(pipe);
    if (result.size() == 0 || result[result.size() - 1] != '\n')
        result += "\n";
    return result;
}

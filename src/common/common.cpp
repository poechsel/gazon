#include <common/common.h>
#include <errno.h>

/** Return a formatted string containing the current value of errno. */
std::string formatError(const std::string& message) {
    std::stringstream error;
    error << message << ": " << ::strerror(errno) << ".";
    return error.str();
}

/** Enforce that the result of a function to be >= 0. */
void enforce(int i) {
    if (i < 0) {
        throw CommandException(formatError("Failed"));
    }
}

uint skipEmptyChars(uint start, std::string const& s) {
    uint end = start;
    for (uint i = start; i < s.size() && std::isspace(s[i]); ++i) {
        end = i + 1;
    }
    return end;
}

uint skipNonEmptyChars(uint start, std::string const& s) {
    uint end = start;
    for (uint i = start; i < s.size() && !std::isspace(s[i]); ++i) {
        end = i + 1;
    }
    return end;
}

uint skipUntil(uint start, std::string const &s, std::string const &chars) {
    uint end = start;
    for (uint i = start; i < s.size() && chars.find(s[i]) == std::string::npos; ++i) {
        end = i + 1;
    }
    return end;
}

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


std::string exec(std::string cmd) {
    std::array<char, 256> buffer;
    std::string result = "";
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    try {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
    } catch (const std::exception& e) {
        pclose(pipe);
        // TODO: throw exception
        result = "";
    }
    pclose(pipe);
    if (result.size() == 0 || result[result.size() - 1] != '\n')
        result += "\n";
    return result;
}

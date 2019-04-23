#include <common/common.h>

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

uint skipArg(uint start, std::string const &s) {
    uint end = start;
    if (s[start] == '\'') {
        end = start + 1;
        for (uint i = start + 1; i < s.size() && s[i] != '\''; ++i) {
            end = i + 1;
        }
        return end+1;
    } else if (s[start] == '"') {
        end = start + 1;
        for (uint i = start + 1; i < s.size() && !(s[i] == '"' && s[i-1] != '\\'); ++i) {
            end = i + 1;
        }
        return end+1;
    } else {
        end = skipNonEmptyChars(start, s);
    }
    return end;
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
    FILE* pipe = popen(cmd.c_str(), "r");
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

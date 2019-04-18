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
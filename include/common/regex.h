#pragma once

#include <common/regexnfa.h>



class Regex {
public:
    Regex(std::string pattern);
    bool match(std::string const &string);
    bool asMatch(std::string const &string);
private:
    std::string m_pattern;
    RegexNfa<uint16_t> m_nfa;
};

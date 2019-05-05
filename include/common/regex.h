#pragma once

#include <common/regexnfa.h>
#include <common/regexdfa.h>



class Regex {
public:
    /* Build a regex from a given pattern */
    Regex(std::string pattern);

    /* Return true if the regex matches string */
    bool match(std::string const &string);
private:
    std::string m_pattern;
    RegexNfa m_nfa;
    RegexDfa m_dfa;
};

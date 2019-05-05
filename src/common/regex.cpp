#include <common/regex.h>
#include <string.h>

#include <vector>
#include <string>
#include <iostream>
#include <stack>
#include <common/common.h>
#include <limits>
#include <tuple>



Regex::Regex(std::string pattern) {
    RegexPostfix post(pattern);
    m_nfa.from(post);
    m_dfa.linkNfa(&m_nfa);
}

bool Regex::match(std::string const &s) {
    return m_dfa.match(s);
}

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
    std::cout<<pattern<<"\n";
    RegexPostfix post(pattern);
    std::cout<<post.debug()<<"\n";
    m_nfa.from(post);
    m_dfa.linkNfa(&m_nfa);
    std::cout<<"size of the nfa: "<<m_nfa.getNStates()<<"\n";
}

bool Regex::match(std::string const &s) {
    return m_dfa.match(s);
}

bool Regex::asMatch(std::string const &s) {
    return m_dfa.asMatch(s);
}

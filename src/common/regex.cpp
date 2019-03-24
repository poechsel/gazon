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
    std::cout<<"size of the nfa: "<<m_nfa.getNStates()<<"\n";
}

bool Regex::match(std::string const &s) {
    return m_nfa.match(s);
}

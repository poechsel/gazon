#pragma once
#include <string>

#include <vector>
#include <string>
#include <iostream>
#include <stack>
#include <common/common.h>
#include <limits>
#include <tuple>


struct RegexException : public std::runtime_error {
public:
    RegexException(std::string m): std::runtime_error(m) {
        std::cout<<m<<"\n";
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

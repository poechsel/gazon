#pragma once

#include <stdexcept>
#include <exception>
#include <cctype>

struct NetworkingException : public std::runtime_error {
public:
    NetworkingException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

typedef unsigned int uint;

uint skipEmptyChars(uint start, std::string const& s);

uint skipNonEmptyChars(uint start, std::string const& s);

uint skipArg(uint start, std::string const& s);

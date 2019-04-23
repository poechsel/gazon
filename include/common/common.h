#pragma once

#include <stdexcept>
#include <exception>
#include <cctype>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <common/defer.h>

struct NetworkingException : public std::runtime_error {
public:
    NetworkingException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

typedef unsigned int uint;

/* Return the index of the first non empty char in `s` after `start` */
uint skipEmptyChars(uint start, std::string const& s);

/* Return the index of the first empty char in `s` after `start` */
uint skipNonEmptyChars(uint start, std::string const& s);

/* Return the index of the first char in `prohibited` */
uint skipUntil(uint start, std::string const &s, std::string const &chars);

uint skipArg(uint start, std::string const& s);

std::vector<std::string> splitString(const std::string &s, char sep);


#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

std::string exec(std::string cmd);

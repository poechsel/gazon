#pragma once

#include <stdexcept>
#include <exception>
#include <cctype>
#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <string>
#include <common/defer.h>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <memory>

typedef unsigned int uint;

/** Exception used when dealing with network packets. */
struct NetworkingException : public std::runtime_error {
public:
    NetworkingException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

/** Exception used when parsing or running commands. */
struct CommandException : public std::runtime_error {
public:
    CommandException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

/** Return a formatted string containing the current value of errno. */
std::string formatError(const std::string& message);

/** Quote a string to safely insert it inside a shell command. */
std::string quote(const std::string &s);

/** Enforce that the result of a function to be >= 0. */
void enforce(int i);

/** Return the index of the first non empty char in `s` after `start`. */
uint skipEmptyChars(uint start, std::string const& s);

/** Return the index of the first empty char in `s` after `start`. */
uint skipNonEmptyChars(uint start, std::string const& s);

/** Return the index of the first char in `prohibited`. */
uint skipUntil(uint start, std::string const &s, std::string const &chars);

/** Return the position of the end of the argument. */
std::tuple<uint, bool> skipArg(uint start, std::string const& s);

/** Split a string into a vector of strings, using a given `sep`. */
std::vector<std::string> splitString(const std::string &s, char sep);

/** Execute a shell command using popen. */
std::string exec(std::string cmd);

#pragma once

#include <exception>

struct NetworkingException : public std::runtime_error {
public:
    NetworkingException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

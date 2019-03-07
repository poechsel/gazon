#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <iostream>

struct CommandException : public std::runtime_error {
public:
    CommandException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};


class Context {
};

typedef std::vector<std::string> CommandArgs;

class Command {
public:
    virtual void executeServer(Context *context) = 0;
    virtual void executeClient(Context *context) = 0;
};



/* The code for the factory is strongly inspired by Nori */
class CommandFactory {
public:
    typedef std::function<Command*(const CommandArgs &)> Constructor;


    static void registerC(const std::string &name, Constructor c);

    static Command* create(const std::string &name, const CommandArgs &args) {
        if (!m_constructors || m_constructors->find(name) == m_constructors->end()) {
            throw CommandException("Command " + name + " not found");
        }
        return (*m_constructors)[name](args);
    }

private:
    static std::unordered_map<std::string, Constructor> *m_constructors;
};

/// Macro for registering an object constructor with the \ref NoriObjectFactory
#define REGISTER_COMMAND(cls, name)                              \
    cls *cls ##_create(const CommandArgs &args) {                  \
        return new cls(args);                                       \
    }                                                               \
    static struct cls ##_{                                          \
        cls ##_() {                                                 \
            CommandFactory::registerC(name, cls ##_create);  \
        }                                                           \
    } cls ##__GAZON_;

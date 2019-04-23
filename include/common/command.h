#pragma once

#include <vector>
#include <common/variant.h>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <common/common.h>
#include <common/socket.h>
#include <common/path.h>
#include <common/config.h>

struct CommandException : public std::runtime_error {
public:
    CommandException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

/* Enumeration of possible types for the arguments */
enum ArgTypes {
    ARG_PATH,
    ARG_INT,
    ARG_STR,
};

typedef std::vector<std::string> CommandArgsString;
typedef Variant<int, std::string, Path> CommandArg;
typedef std::vector<CommandArg> CommandArgs;

typedef std::vector<ArgTypes> Specification;

enum MiddlewareTypes {
                      // a command can always be used
                      MIDDLEWARE_NONE,
                      // a command can only be used if a user is logged
                      MIDDLEWARE_LOGGED,
                      // a command can only be used when a user is attempting to log
                      MIDDLEWARE_LOGGING,
};

class Context {
public:
    bool isLogged;
    std::string user;
    Path path;

    Context(): isLogged(false), user(""), path("") {
    }
};

class Command {
public:
    Command(MiddlewareTypes middlewareTypes):
        m_middlewareTypes(middlewareTypes) {
    }
    virtual void execute(Socket &socket, Context &context, const CommandArgs& args) = 0;
    virtual Specification getSpecification() const = 0;
    bool middleware(Context &context) const;
    virtual ~Command(){}
protected:
    MiddlewareTypes m_middlewareTypes;
};

/* The code for the factory is strongly inspired by Nori */
class CommandFactory {
public:
    typedef std::function<Command*()> Constructor;

    static void registerC(const std::string &name, Constructor c);

    static Command* create(const std::string &name) {
        if (!m_constructors || m_constructors->find(name) == m_constructors->end()) {
            throw CommandException("Command " + name + " not found");
        }
        return (*m_constructors)[name]();
    }

    static void destroy() {
        if (m_constructors != nullptr)
            delete m_constructors;
    }

private:
    static std::unordered_map<std::string, Constructor> *m_constructors;
};

/* return the command corresponding to the input line `line` and the index of
   the start of the args.
   Ex:
   (6, command_hello) = commandFromInput("hello a b c d e");
*/
std::tuple<Command*, CommandArgsString> commandFromInput(std::string const& line);

/* Return true if the argument list matches the specification */
CommandArgs convertAndTypecheckArguments(const Context &context, Specification const& spec, CommandArgsString const &args);

// /* Evaluate the command express in line [line] */
// void evaluateCommandFromLine(Context *context, std::string const& line, bool onServer = true);

/// Macro for registering an object constructor with the \ref NoriObjectFactory
#define REGISTER_COMMAND(cls, name)                              \
    cls *cls ##_create() {                  \
        return new cls();                                       \
    }                                                               \
    static struct cls ##_{                                          \
        cls ##_() {                                                 \
            CommandFactory::registerC(name, cls ##_create);  \
        }                                                           \
    } cls ##__GAZON_;

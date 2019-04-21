#pragma once

#include <vector>
#include <common/variant.h>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <common/common.h>
#include <common/socket.h>
#include <common/path.h>

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

// TODO(liautaud)
class Context {};

class Command {
public:
    virtual void execute(Socket &socket, Context &context, const CommandArgs& args) = 0;
    virtual Specification getSpecification() const = 0;
    virtual ~Command(){}
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
CommandArgs convertAndTypecheckArguments(Specification const& spec, CommandArgsString const &args);

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

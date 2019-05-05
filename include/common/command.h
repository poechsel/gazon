#pragma once

#include <vector>
#include <common/variant.h>
#include <map>
#include <regex>
#include <functional>
#include <iostream>
#include <common/common.h>
#include <common/context.h>
#include <common/socket.h>
#include <common/path.h>

/* Enumeration of possible types for the arguments. */
enum ArgTypes {
    ARG_PATH,
    ARG_INT,
    ARG_STR,
    ARG_HOSTNAME,
    ARG_PATTERN,
};

/* Abstract specification of the types of argument a command expects. */
typedef std::vector<ArgTypes> Specification;

/* List of unparsed string arguments. */
typedef std::vector<std::string> CommandArgsString;

/* A single parsed argument (either an int, a string or a path). */
typedef Variant<int, std::string, Path> CommandArg;

/* List of parsed arguments. */
typedef std::vector<CommandArg> CommandArgs;

/* Types of middlewares that a command can expect. */
enum MiddlewareTypes {
    MIDDLEWARE_NONE,       //< The command can always be used.
    MIDDLEWARE_LOGGING,    //< The user must have started a login attempt.
    MIDDLEWARE_LOGGED,     //< The user must be logged in.
    MIDDLEWARE_LOGGED_OUT, //< The user must be logged out.
};

/**
 * Abstract command class.
 *
 * All actual commands (e.g. `grep` or `ls`) inherit from this class,
 * and implement the execute() and getSpecification() methods as well
 * as define which middleware they would like to use.
 */
class Command {
public:
    Command(MiddlewareTypes middlewareTypes): m_middlewareTypes(middlewareTypes) {}
    virtual ~Command(){}

    /**
     * Execute the command on the server.
     *
     * This method is only called if the declared middleware allowed the request
     * to go through, and if the arguments passed to the command were formatted
     * correctly according to the declared specification.
     *
     * @param socket  The socket used to communicate with the client.
     * @param context The context of the current session.
     * @param args    The parsed arguments of the command.
     */
    virtual void execute(Socket &socket, Context &context, const CommandArgs& args) = 0;

    /**
     * Return the type of the arguments that the command expects.
     *
     * For instance, returning {ARG_PATH, ARG_INT} will mean that the command
     * will only accept valid paths (relative to the base directory) followed
     * by decimal integers.
     */
    virtual Specification getSpecification() const = 0;

    /**
     * Check if a context passes the specified middleware.
     * Set the value of the variable pointed by status accordingly (0 or 1).
     */
    void middleware(Context &context, int *status) const;

    /**
     * Try to parse the arguments according to the specification.
     *
     * @return CommandArgs      The parsed arguments.
     * @throw  CommandException If some of the arguments didn't match the spec.
     */
    CommandArgs convertAndTypecheckArguments(
        const Context &context,
        CommandArgsString const &args) const;

protected:
    MiddlewareTypes m_middlewareTypes;
};

/**
 * A utility class to register commands via a macro.
 *
 * The code for this is strongly inspired by Nori, an educational raytracer
 * available at https://github.com/cs440-epfl/nori-base-2019/.
 */
class CommandFactory {
public:
    typedef std::function<Command*()> Constructor;

    /** Add a command constructor to the list of constructors. */
    static void registerCommand(const std::string &name, Constructor c);

    /** Try to create an instance of the command with the given name. */
    static Command* create(const std::string &name) {
        if (!m_constructors || m_constructors->find(name) == m_constructors->end()) {
            throw CommandException("command " + name + " not found.");
        }
        return (*m_constructors)[name]();
    }

    /** Free the list of constructors. */
    static void destroy() {
        if (m_constructors != nullptr) {
            delete m_constructors;
        }
    }

private:
    static std::unordered_map<std::string, Constructor> *m_constructors;
};

/**
 * Try to parse a raw input line into a command and its arguments.
 *
 * For instance, commandFromInput("foo bar") will return a pointer to
 * a FooCommand as well as the list ["bar"].
 *
 * @return A pointer to the command instance, and a list of unparsed
 *         string arguments.
 * @throw  CommandException If the parsing fails because the desired
 *         command is not registered in the factory.
 */
std::tuple<Command*, CommandArgsString> commandFromInput(std::string const& line);

/// Macro for registering a command constructor into the CommandFactory.
#define REGISTER_COMMAND(cls, name)                               \
    cls *cls ##_create() {                                        \
        return new cls();                                         \
    }                                                             \
    static struct cls ##_{                                        \
        cls ##_() {                                               \
            CommandFactory::registerCommand(name, cls ##_create); \
        }                                                         \
    } cls ##__GAZON_;

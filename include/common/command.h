#pragma once

#include <vector>
#include <common/variant.h>
#include <map>
#include <functional>
#include <iostream>
#include <mutex>
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

/* Enumeration of possible types for the arguments. */
enum ArgTypes {
    ARG_PATH,
    ARG_INT,
    ARG_STR,
    ARG_DOMAIN,
};

typedef std::vector<std::string> CommandArgsString;
typedef Variant<int, std::string, Path> CommandArg;
typedef std::vector<CommandArg> CommandArgs;

typedef std::vector<ArgTypes> Specification;

enum MiddlewareTypes {
    MIDDLEWARE_NONE,       //< The command can always be used.
    MIDDLEWARE_LOGGING,    //< The user must have started a login attempt.
    MIDDLEWARE_LOGGED,     //< The user must be logged in.
    MIDDLEWARE_LOGGED_OUT, //< The user must be logged out.
};

/** The context of a single user session. */
class Context {
public:
    // Static map of all the logged in users (and number of active sessions).
    static std::map<std::string, unsigned int> logged;
    static std::mutex loggedMutex;

    /// Whether the user is currently logged in.
    bool isLogged = false;

    /// The username that the user is logged in as.
    std::string user = "";

    /// The relative path of the current working directory.
    Path relativePath;

    Context(): relativePath("") {}

    /** Return the absolute path of the current working directory. */
    Path getAbsolutePath() const {
        return Config::base_directory + relativePath;
    }

    /** Try to log the user in, throw a CommandException otherwise. */
    void login(const std::string &username, const std::string &password) {
        if (!Config::isUserPwdValid(username, password)) {
            throw CommandException("Invalid credentials.");
        }

        user = username;
        isLogged = true;
        relativePath = Path("");

        // If the entry doesn't exist, it will be zero-initialized.
        std::unique_lock<std::mutex> lock(loggedMutex);
        logged[username]++;
    }

    /** Try to log the user out, or fail silently. */
    void logout() {
        if (!isLogged) {
            return;
        }

        std::string username = user;
        user = "";
        isLogged = false;
        relativePath = Path("");

        std::unique_lock<std::mutex> lock(loggedMutex);
        logged[username]--;
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

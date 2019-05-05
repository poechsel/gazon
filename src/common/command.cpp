#include <common/command.h>

/// Regular expression which matches RFC 1123 hostnames.
static std::regex hostnameRegex("^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*"
                                "([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$");

// Initialization of static variables.
std::map<std::string, unsigned int> Context::logged;
std::mutex Context::loggedMutex;
std::unordered_map<std::string, CommandFactory::Constructor> *CommandFactory::m_constructors = nullptr;

/** Ensures that the passed string doesn't contain \0. */
void checkContainsNoNullBytesAndThrow(const std::string &s) {
    for (uint64_t j = 0; j < s.size(); ++j) {
        if (!s[j]) throw CommandException("incorrect path.");
    }
}

/** Try to parse the arguments according to the specification. */
CommandArgs Command::convertAndTypecheckArguments(const Context &context, CommandArgsString const& args) const {
    Path argpath;
    Specification const &spec = getSpecification();
    if (spec.size() != args.size())
        throw CommandException("number of arguments doesn't match.");
    std::vector<CommandArg> converted;
    for (uint64_t npath, i = 0; i < spec.size(); ++i) {
        CommandArg arg;
        switch (spec[i]) {
            case ARG_PATH: { // File path.
                npath++;
                // A path can't contain any null bytes (this is specified by posix!)
                if (args[i].find((char)0) != std::string::npos)
                    throw CommandException("incorrect path.");
                Path argpath(args[i], &context);
                // The path passed as argument can be either relative or absolute, but
                // always to the base directory. In any case, we add it to the relative
                // path in the context. If this argpath is relative, the behavior is as
                // expected. If the argpath is absolute, the concatenation operator will
                // only return the argpath/
                Path relative_path = context.relativePath + argpath;

                // Then we force the path to be relative to the base directory.
                relative_path.setRelative();
                if (relative_path.attemptParentTraversal())
                    throw CommandException("access denied.");
                if (relative_path.length() > 128)
                    throw CommandException("the path is too long.");
                arg.set<Path>(relative_path);
                break;
            }

            case ARG_INT: // Decimal integer.
                try {
                    arg.set<int>(std::stoi(args[i]));
                } catch (const std::exception &e) {
                    throw CommandException("argument should be an int.");
                }
                break;

            case ARG_HOSTNAME: // Hostname, as per RFC 1123.
                if (!std::regex_search(args[i], hostnameRegex))
                    throw CommandException("argument should be a valid domain name.");
                arg.set<std::string>(args[i]);
                break;

            case ARG_STR: // Possibly quoted string.
            case ARG_PATTERN: // Extended grep pattern.
                arg.set<std::string>(args[i]);
                break;

            default:
                throw CommandException("unhandled argument type.");
        }

        converted.push_back(arg);
    }
    return converted;
}

/** Check if a context passes the specified middleware. */
void Command::middleware(Context &context, int *status) const {
    if (m_middlewareTypes != MIDDLEWARE_LOGGING)
        context.isLoggingIn = false;

    switch (m_middlewareTypes) {
        /// No additional checks needed.
        case MIDDLEWARE_NONE:
            *status = true;
            break;

        /// User should have filled their username.
        case MIDDLEWARE_LOGGING:
            if (context.isLoggingIn) {
                *status = true;
            } else {
                context.isLogged = false;
                context.user = "";
                *status = false;
            }
            break;

        /// User should be logged in.
        case MIDDLEWARE_LOGGED:
            *status = context.isLogged;
            break;

        /// User should be logged out.
        case MIDDLEWARE_LOGGED_OUT:
            *status = !context.isLogged;
            break;

        /// Unknown middleware.
        default:
            *status = true;
            break;
    }
}

/** Add a command constructor to the list of constructors. */
void CommandFactory::registerCommand(const std::string &name, Constructor c) {
    if (!m_constructors)
        m_constructors = new std::unordered_map<std::string, CommandFactory::Constructor>();
    (*m_constructors)[name] = c;
}

/** Try to parse a raw input line into a command and its arguments. */
std::tuple<Command*, CommandArgsString> commandFromInput(std::string const& input) {
    uint start = skipEmptyChars(0, input);
    uint stop_command_name = skipNonEmptyChars(start, input);
    std::string command_name = input.substr(start, stop_command_name - start);
    Command* command = CommandFactory::create(command_name);
    CommandArgsString command_args;
    start = stop_command_name;
    while (start < input.size()) {
        start = skipEmptyChars(start, input);
	uint end; bool isEscaped;
	std::tie(end, isEscaped) = skipArg(start, input);
	uint estart = start + isEscaped;
	uint eend = end - isEscaped;
        std::string arg = input.substr(estart, eend - estart);
        start = end;
        if (arg.size() > 0)
            command_args.push_back(arg);
    }
    return make_tuple(command, command_args);
}

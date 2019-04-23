#include <common/command.h>

std::unordered_map<std::string, CommandFactory::Constructor> *CommandFactory::m_constructors = nullptr;

void CommandFactory::registerC(const std::string &name, Constructor c) {
    if (!m_constructors)
        m_constructors = new std::unordered_map<std::string, CommandFactory::Constructor>();
    (*m_constructors)[name] = c;
}

std::tuple<Command*, CommandArgsString> commandFromInput(std::string const& input) {
    uint start = skipEmptyChars(0, input);
    uint stop_command_name = skipNonEmptyChars(start, input);
    std::string command_name = input.substr(start, stop_command_name - start);
    Command* command = CommandFactory::create(command_name);
    CommandArgsString command_args;
    start = stop_command_name;
    while (start < input.size()) {
        start = skipEmptyChars(start, input);
        uint end = skipArg(start, input);
        std::string arg = input.substr(start, end - start);
        start = end;
        command_args.push_back(arg);
    }
    return make_tuple(command, command_args);
}

CommandArgs convertAndTypecheckArguments(const Context &context, Specification const& spec, CommandArgsString const& args) {
    if (spec.size() != args.size())
        throw CommandException("number of arguments doesn't match");
    std::vector<CommandArg> converted; 
    for (uint i = 0; i < spec.size(); ++i) {
        CommandArg arg;
        if (spec[i] == ARG_INT) {
            try {
                arg.set<int>(std::stoi(args[i]));
                break;
            } catch (const std::exception &e) {
                throw CommandException("argument should be an int");
            }
        } else if (spec[i] == ARG_PATH) {
            Path argpath(args[i]);
            /* The path passed as argument can be either relative or absolute, but
               always to the base directory.
               In any case, we add it to the relative path in the context. If
               this argpath is relative, the behavior is as expected. If the argpath
               is absolute, the concatenation operator will only return the argpath */
            Path relative_path = context.relative_path + argpath;
            /* Then we force the path to be relative to the base directory */
            relative_path.setRelative();
            if (relative_path.attemptParentTraversal()) {
                throw CommandException("access denied!");
            }
            if (relative_path.size() > 128) {
                throw CommandException("the path is too long.");
            }
            arg.set<Path>(relative_path);
        } else {
            arg.set<std::string>(args[i]);
        }
        converted.push_back(arg);
    }
    return converted;
}

bool Command::middleware(Context &context) const {
    if (m_middlewareTypes == MIDDLEWARE_NONE)
        return true;

    // While logging we want to switch back to a non logged/logging state
    // if the middleware fails
    if (m_middlewareTypes == MIDDLEWARE_LOGGING) {
        if (context.user != "" and !context.isLogged)
            return true;
        context.isLogged = false;
        context.user = "";
        return false;
    }

    if (m_middlewareTypes == MIDDLEWARE_LOGGED) {
        return context.isLogged;
    }

    return false;
}

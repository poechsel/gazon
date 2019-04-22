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

CommandArgs convertAndTypecheckArguments(Specification const& spec, CommandArgsString const& args) {
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
            Path path(args[i]);
            if (path.isAbsolute()) {
                throw CommandException("we only accept relative paths");
            }
            if (path.attemptParentTraversal()) {
                throw CommandException("detected attempt of directory traversal");
            }
            // TODO: check if the path is not too long
            arg.set<Path>(path);
        } else {
            arg.set<std::string>(args[i]);
        }
        converted.push_back(arg);
    }
    return converted;
}

// void evaluateCommandFromLine(Context *context, std::string const& line, bool onServer) {
//     Command *command;
//     CommandArgs command_args;
//     std::tie(command, command_args) = commandFromInput(line);

//     if (typecheckArguments(command->getSpecification(), command_args)) {
//         if (onServer)
//             command->executeServer(context, command_args);
//         else
//             command->executeClient(context, command_args);
//     } else {
//         // TODO: raise an exception
//         std::cout<<"TYPECHECKING error\n";
//     }
// }

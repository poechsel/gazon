#include <common/command.h>

std::unordered_map<std::string, CommandFactory::Constructor> *CommandFactory::m_constructors = nullptr;

void CommandFactory::registerC(const std::string &name, Constructor c) {
    if (!m_constructors)
        m_constructors = new std::unordered_map<std::string, CommandFactory::Constructor>();
    (*m_constructors)[name] = c;
}

std::tuple<Command*, CommandArgs> commandFromInput(std::string const& input) {
    uint start = skipEmptyChars(0, input);
    uint stop_command_name = skipNonEmptyChars(start, input);
    std::string command_name = input.substr(start, stop_command_name - start);
    Command* command = CommandFactory::create(command_name);
    CommandArgs command_args;
    start = stop_command_name;
    while (start < input.size()) {
        start = skipEmptyChars(start, input);
        uint end = skipArg(start, input);
        std::string arg = input.substr(start, end - start);
        start = end;
        command_args.push_back(arg);
    }
    return {command, command_args};
}

bool typecheckArguments(Specification const& spec, CommandArgs const& args) {
    if (spec.size() != args.size())
        return false;
    for (uint i = 0; i < spec.size(); ++i) {
        switch (spec[i]) {
        case ARG_INT:
            try {
                std::stoi(args[i]);
                break;
            } catch (const std::exception &e) {
                return false;
            }

        case ARG_PATH:
            for (auto c : args[i])
                if (std::isspace(c))
                    return false;
            break;

        case ARG_STR:
            break;
        }
    }
    return true;
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

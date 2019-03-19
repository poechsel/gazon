#include <common/command.h>

class HelloCommand : public Command {
public:
    virtual void executeServer(Context *context, const CommandArgs &args) {
        std::cout<<"Hello on the server ["<<args[0]<<"]\n";
    }

    virtual void executeClient(Context *context, const CommandArgs &args) {
        std::cout<<"Hello on the client\n";
    }

    virtual Specification getSpecification() const {
        return {ARG_STR};
    }
private:
};

REGISTER_COMMAND(HelloCommand, "hello");

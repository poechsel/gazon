#include <common/command.h>

class HelloCommand : public Command {
public:
    HelloCommand(const CommandArgs &args) {
        m_name = args[0];
    }

    virtual void executeServer(Context *context) {
        std::cout<<"Hello on the server ["<<m_name<<"]\n";
    }

    virtual void executeClient(Context *context) {
        std::cout<<"Hello on the client\n";
    }
private:
    std::string m_name;
};

REGISTER_COMMAND(HelloCommand, "hello");

#include <common/command.h>
#include <mutex>

class WCommand : public Command {
public:
    WCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &socket, Context&, const CommandArgs&) {
        std::string users = "";
        std::unique_lock<std::mutex> lock(Context::loggedMutex);

        // The std::map is sorted by alphabetical key order.
        for (auto &pair : Context::logged) {
            if (pair.second > 0) {
                users += pair.first;
                users += " ";
            }
        }

        lock.unlock();

        if (users.length() > 0) users.pop_back();
        socket << users << std::endl;
    }

    Specification getSpecification() const {
        return {};
    }
private:
};

REGISTER_COMMAND(WCommand, "w");

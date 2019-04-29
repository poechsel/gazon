#include <common/command.h>
#include <common/filesystem.h>
#include <common/file.h>
#include <common/socket.h>

#include <sys/socket.h>

using std::cout;
using std::endl;

class GetCommand : public Command {
public:
    GetCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &socket, Context &context, const CommandArgs &args) {
        // Check that a thread pool is available in the context.
        if (context.fpool == nullptr) {
            throw CommandException("Cannot schedule a file download.");
        }

        // We will use socket.h functions here directly as the code would
        // be harder to write using the Socket abstraction. 
        int ffd = ::socket(AF_INET, SOCK_STREAM, 0);
        enforce(ffd);

        // Let the OS choose a random port by binding to port 0.
        Address addr = Socket::parseAddress("127.0.0.1", 0);
        socklen_t addrLength = sizeof(addr);
        enforce(bind(ffd, (struct sockaddr *) &addr, addrLength));
        enforce(getsockname(ffd, (struct sockaddr *) &addr, &addrLength));
        int port = ntohs(addr.sin_port);

        // Schedule on a separate pool to allow a more fine-grained
        // control of the number of parallel file transfers possible.
        context.fpool->schedule(port, [&socket, &context, args, port, ffd]() {
            try {
                Path path = context.getAbsolutePath() + args[0].get<Path>();
                File input = Filesystem::read(path);
                int size = input.size;

                // Signal to the client that it can start to pull data.
                socket << "get port: " << std::to_string(port)
                       << " size: " << std::to_string(size) << endl;

                enforce(listen(ffd, SOMAXCONN) < 0);
                cout << "[INFO] Awaiting connection for file "
                     << path.string() << "." << endl;

                int cfd = accept(ffd, nullptr, nullptr);
                enforce(cfd);
                cout << "[INFO] Started sending file " << path.string() << "." << endl;
                
                char buffer[4096];
                int bytesRead;
                while ((bytesRead = input.read(buffer, 4096))) {
                    enforce(write(cfd, buffer, bytesRead));
                }

                // Close everything once the entire file is sent.
                cout << "[INFO] Sent file " << path.string() << "." << endl;
                close(cfd);
                close(ffd);
                input.close();
            } catch (const std::exception &e) {
                socket << "Error: " << e.what() << endl;
            }
        });
    }

    Specification getSpecification() const {
        return {ARG_PATH};
    }
private:
};

REGISTER_COMMAND(GetCommand, "get");

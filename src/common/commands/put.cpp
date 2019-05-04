#include <common/command.h>
#include <common/filesystem.h>
#include <common/file.h>
#include <common/socket.h>

#include <sys/socket.h>

using std::cout;
using std::endl;

class PutCommand : public Command {
public:
    PutCommand(): Command(MIDDLEWARE_LOGGED) {}

    void execute(Socket &socket, Context &context, const CommandArgs &args) {
        // Check that a thread pool is available in the context.
        if (context.fpool == nullptr) {
            throw CommandException("Cannot schedule a file upload.");
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

        FileKeyThread key(args[0].get<Path>().string());
            // Schedule on a separate pool to allow a more fine-grained
            // control of the number of parallel file transfers possible.
        context.fpool->schedule(key,
                                [&socket, &context, args, port, ffd]() {
            try {
                Path path = context.getAbsolutePath() + args[0].get<Path>();
                TemporaryFile output = Filesystem::createFile(path);
                int size = args[1].get<int>();

                // Signal to the client that it can start to push data.
                socket << "put port: " << std::to_string(port)
                       << " path: " << args[0].get<Path>().string() << endl;

                enforce(listen(ffd, SOMAXCONN) < 0);
                cout << "[INFO] Awaiting connection for file "
                     << path.string() << "." << endl;

                int cfd = accept(ffd, nullptr, nullptr);
                enforce(cfd);

                cout << "[INFO] Awaiting data for file "
                     << path.string() << "." << endl;

                char buffer[4096];
                int bytesRead;
                int totalRead = 0;
                while ((bytesRead = read(cfd, buffer, 4096)) > 0 && totalRead < size) {
                    output.write(buffer, std::min(size - totalRead, bytesRead));
                    totalRead += bytesRead;
                }

                if (totalRead >= size) {
                    Filesystem::commit(output);
                    cout << "[INFO] Received file " << path.string() << "." << endl;
                } else {
                    output.close();
                    socket << "Error: did not receive the whole file ("
                           << std::to_string(totalRead) << " bytes read)." << endl;
                    cout << "[ERROR] Did not receive the whole file for "
                         << path.string() << " (" << totalRead << ")." << endl;
                }

                close(cfd);
                close(ffd);
            } catch (const std::exception &e) {
                if (socket.getFd() >= 0) {
                    socket << "Error: " << e.what() << endl;
                }
            }
        });
    }

    Specification getSpecification() const {
        return {ARG_PATH, ARG_INT};
    }
private:
};

REGISTER_COMMAND(PutCommand, "put");

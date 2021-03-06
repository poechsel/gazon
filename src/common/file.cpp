#include <common/file.h>
using namespace std;

/** Close the currently opened file. */
void File::close() {
    if (stream.is_open()) {
        stream.close();
        size = 0;
    }
}

/** Read a line from the file and store it in a buffer. */
bool File::getLine(std::string &out) {
    return static_cast<bool>(getline(stream, out));
}

/** Read at most n characters from the file. */
unsigned int File::read(char* buffer, unsigned int count) {
    stream.read(buffer, count);
    return stream.gcount();
}

/** Close the temporary file. */
void TemporaryFile::close() {
    if (stream.is_open()) {
        stream.close();
    }
}

/** Commit the temporary file to its real path. */
void TemporaryFile::commit() {
    close();

    std::string temp = tempPath.string();
    std::string real = realPath.string();
    if (rename(temp.c_str(), real.c_str()) != 0) {
        throw FilesystemException("Could not commit temporary file.");
    }
}

/** Write raw data to the file. */
void TemporaryFile::write(const char* buffer, unsigned int count) {
    stream.write(buffer, count);
}
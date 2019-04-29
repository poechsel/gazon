#pragma once

#include <common/common.h>
#include <common/path.h>
#include <iostream>
#include <fstream>
#include <cstdio>

struct FilesystemException : public std::runtime_error {
public:
    FilesystemException(std::string m): std::runtime_error(m) {
    }

	const char * what () const throw () {
    	return std::runtime_error::what();
    }
};

class File {
public:
    File(File&&) = default;
    File(const Path &p): path("") { open(p); }
    ~File() { close(); }

    /** Open the file at the given path. */
    void open(const Path&);

    /** Close the currently opened file. */
    void close();

    /** Read a line from the file and store it in a buffer. */
    bool getLine(std::string&);

    /**
     * Read at most n characters from the file.
     *
     * @param buffer The buffer in which to store the characters.
     * @param count  The maximum number of characters to read.
     * @return The number of characters actually read.
     */
    unsigned int read(char* buffer, unsigned int count);

    Path path; ///< Absolute path to the file.
    long size; ///< Size of the file, in bytes.

private:
    std::ifstream stream; ///< Binary input stream of the file.
};

class TemporaryFile {
public:
    TemporaryFile(TemporaryFile&&) = default;
    TemporaryFile(const Path &tempPath, const Path &realPath):
        tempPath(""),
        realPath(realPath) { open(tempPath); }
    ~TemporaryFile() { close(); }

    /** Open the given temporary file. */
    void open(const Path &path);

    /** Close the temporary file. */
    void close();

    /** Commit the temporary file to its real path. */
    void commit();

    /**
     * Write raw data to the file.
     *
     * @param buffer The source buffer for the data.
     * @param count  The number of bytes to write.
     */
    void write(const char* buffer, unsigned int count);

    Path tempPath; ///< The path to the temporary storage.
    Path realPath; ///< The path to which the file will be commited.

private:
    std::ofstream stream; ///< Binary input stream of the temporary file.
};

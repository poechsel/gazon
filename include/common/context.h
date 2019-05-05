#pragma once

#include <iostream>
#include <mutex>
#include <common/path.h>
#include <common/common.h>
#include <common/config.h>
#include <common/threadpool.h>

/** The context of a single user session. */
class Context {
public:
    // Static map of all the logged in users (and number of active sessions).
    static std::map<std::string, unsigned int> logged;
    static std::mutex loggedMutex;

    /// Whether the user is currently logged in.
    bool isLogged = false;
    bool isLoggingIn = false;

    /// The username that the user is logged in as.
    std::string user = "";

    /// The relative path of the current working directory.
    Path relativePath;

    /// The thread pool in which to allocate file jobs.
    ThreadPool<FileKeyThread, 8> *fpool = nullptr;

    Context(): relativePath("") {}

    /** Return the absolute path of the current working directory. */
    Path getAbsolutePath() const {
        return Config::base_directory + relativePath;
    }

    /** Try to log the user in, throw a CommandException otherwise. */
    void login(const std::string &username, const std::string &password) {
        if (!Config::isUserPwdValid(username, password)) {
            throw CommandException("authentication failed.");
        }

        user = username;
        isLogged = true;
        isLoggingIn = false;
        relativePath = Path("");

        // If the entry doesn't exist, it will be zero-initialized.
        std::unique_lock<std::mutex> lock(loggedMutex);
        logged[username]++;
    }

    /** Try to log the user out, or fail silently. */
    void logout() {
        if (!isLogged) {
            return;
        }

        std::string username = user;
        user = "";
        isLogged = false;
        isLoggingIn = false;
        relativePath = Path("");

        std::unique_lock<std::mutex> lock(loggedMutex);
        logged[username]--;
    }
};
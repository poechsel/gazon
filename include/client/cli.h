#pragma once

#include <string>
#include <iostream>
#include <mutex>

class Cli {
public:
    Cli(std::string const &start_input = ">> ") :
        m_start_input(start_input) {
    }

    void showError(std::string const &error);

    std::string readInput();

private:
    std::mutex m_mutex;
    std::string m_start_input;
};

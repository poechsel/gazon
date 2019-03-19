#include <client/cli.h>

void Cli::showError(std::string const &error) {
    std::unique_lock<std::mutex> lock(m_mutex);
    std::cout << "\n" << error << "\n";
    std::cout << m_start_input << std::flush;
}

std::string Cli::readInput() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        std::cout << m_start_input << std::flush;
    }
    std::string input;
    std::getline(std::cin, input);
    return input;
}

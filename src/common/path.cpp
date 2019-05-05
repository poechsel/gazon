#include <common/path.h>
#include <common/command.h>

unsigned int Path::size() const {
    return m_parts.size();
}

std::string getHome(const Context *context) {
    char name[32];
    snprintf(name, 32, context->user.c_str(), 42);
    return name;
}


Path::Path(std::string const &path_str, const Context *context)
{
    if (path_str[0] == '/') {
        m_is_absolute = true;
    } else {
        m_is_absolute = false;
    }
    m_parts.clear();
    auto parts = splitString(path_str, '/');
    for (const std::string cpart : parts) {
        if (cpart == "." || cpart == "") {
            continue;
        } else if (cpart == ".."
                   && m_parts.size() > 0
                   && m_parts[m_parts.size() - 1] != "..") {
            m_parts.pop_back();
        } else if (cpart == "~" && context) {
            m_parts.push_back("home");
            auto current_user = getHome(context);
            m_parts.push_back(current_user);
        } else {
            m_parts.push_back(cpart);
        }
    }
    reconstructStringView();
}

bool Path::isAbsolute() const {
    return m_is_absolute;
}

// return true if this path attempts to traverse a parent directory
// (for exemple foo/../../foo)
bool Path::attemptParentTraversal() const {
    int depth = 0;
    for (auto part : m_parts) {
        if (part == "..")
            depth --;
        else
            depth ++;
        if (depth < 0)
            return true;
    }
    return false;
}

std::string Path::string() const {
    return m_string;
}

size_t Path::length() const {
    return m_string.size();
}

Path& Path::operator+=(const std::string &next_part) {
    if (addPathPart(next_part)) {
        reconstructStringView();
    }
    return *this;
}

Path& Path::operator+=(const Path &other) {
    if (other.isAbsolute()) {
        m_parts = other.m_parts;
        m_string = other.m_string;
    } else {
        bool needReconstruction = false;
        for (const auto part : other.m_parts)
            needReconstruction |= addPathPart(part);
        if (needReconstruction)
            reconstructStringView();
    }
    return *this;
}

void Path::setRelative() {
    m_is_absolute = false;
}

void Path::reconstructStringView() {
    m_string = "";
    for (unsigned int i = 0; i < m_parts.size(); ++i) {
        m_string += ((i > 0 || m_is_absolute) ? "/" : "") + m_parts[i];
    }
}

bool Path::addPathPart(const std::string &next_part) {
    if (next_part == ".."
        && m_parts.size() > 0
        && m_parts[m_parts.size() - 1] != "..") {
        m_parts.pop_back();
        return true;
    } else if (next_part == ".") {
        return false;
    } else {
        m_parts.push_back(next_part);
        return true;
    }
}


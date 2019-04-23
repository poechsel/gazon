#include <common/command.h>
#include <common/filesystem.h>
#include <common/regex.h>


char ftypelet (mode_t bits)
{
    if (S_ISREG (bits))
        return '-';
    if (S_ISDIR (bits))
        return 'd';

    return '?';
}

void strmode (mode_t mode, char *str)
{
    str[0] = ftypelet (mode);
    str[1] = mode & S_IRUSR ? 'r' : '-';
    str[2] = mode & S_IWUSR ? 'w' : '-';
    str[3] = (mode & S_ISUID
              ? (mode & S_IXUSR ? 's' : 'S')
              : (mode & S_IXUSR ? 'x' : '-'));
    str[4] = mode & S_IRGRP ? 'r' : '-';
    str[5] = mode & S_IWGRP ? 'w' : '-';
    str[6] = (mode & S_ISGID
              ? (mode & S_IXGRP ? 's' : 'S')
              : (mode & S_IXGRP ? 'x' : '-'));
    str[7] = mode & S_IROTH ? 'r' : '-';
    str[8] = mode & S_IWOTH ? 'w' : '-';
    str[9] = (mode & S_ISVTX
              ? (mode & S_IXOTH ? 't' : 'T')
              : (mode & S_IXOTH ? 'x' : '-'));
    str[10] = ' ';
    str[11] = '\0';
}

int timespec_cmp (struct timespec a, struct timespec b)
{
    if (a.tv_sec < b.tv_sec)
        return -1;
    if (a.tv_sec > b.tv_sec)
        return 1;

    return a.tv_nsec - b.tv_nsec;
}

void gettime (struct timespec *ts)
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
}

bool needsQuoting(std::string name) {
    for (auto c : name) {
        if (c <= 0x20 || c >= 0x7f)
            return true;
    }
    return false;
}

constexpr unsigned int FORMAT_STR_MAXLEN = 128;

struct Lsdata {
    std::string mode;
    std::string date;
    std::string group;
    std::string user;
    std::string name;
    std::string nlink;
    std::string size;
};

struct Lsmaxlength {
    int mode;
    int date;
    int group;
    int user;
    int name;
    int nlink;
    int size;

    void update(const Lsdata &filedata) {
        mode = std::max(mode, (int)filedata.mode.size());
        date = std::max(date, (int)filedata.date.size());
        group = std::max(group, (int)filedata.group.size());
        user = std::max(user, (int)filedata.user.size());
        name = std::max(name, (int)filedata.name.size());
        nlink = std::max(nlink, (int)filedata.nlink.size());
        size = std::max(size, (int)filedata.size.size());
    }
};

class Lsformatstr {
private:
    std::string m_format;

public:
    Lsformatstr(const Lsmaxlength &length) {
        char format_str_chr[FORMAT_STR_MAXLEN];
        snprintf(format_str_chr, FORMAT_STR_MAXLEN,
                 "%%s%%%ds %%-%ds %%-%ds %%%ds %%s",
                 length.nlink,
                 length.user,
                 length.group,
                 length.size
                 );
        m_format = std::string(format_str_chr);
    }

    std::string getFormat(std::string path, bool quotingExists, bool needsQuoting) {
        if (quotingExists) {
            if (needsQuoting) {
                return m_format + " '" + path + "'\n";
            } else {
                return m_format + "  " + path + "\n";
            }
        } else {
            return m_format + " " + path + "\n";
        }
    }

    /* Given an instance of ldata output the correct ls line in `output`.*/
    void dump(const Lsdata &lsdata, size_t n, char *output, bool quotingExists) {
        std::string name = lsdata.name;
        std::string format = getFormat(name, quotingExists, needsQuoting(name));
        snprintf(output, n,
                 format.c_str(),
                 lsdata.mode.c_str(),
                 lsdata.nlink.c_str(),
                 lsdata.user.c_str(),
                 lsdata.group.c_str(),
                 lsdata.size.c_str(),
                 lsdata.date.c_str()
                 );
    }
};

void timeToStr(time_t time, char* time_str, unsigned int n);

class LsCommand : public Command {
public:
    LsCommand(): Command(MIDDLEWARE_LOGGED) {}
    static std::array<std::string, 24> date_fmt;

    void my_ls(Path path, Socket &socket) {
        std::vector<Lsdata> files;
        Lsmaxlength length = {0, 0, 0, 0, 0, 0, 0};
        int nblocks = 0;
        bool quotingExists = false;
        /* We start by storing in an array every element
           that will be part of the final output. We also
           keep track of the width of each column */
        {
            Filesystem::lock();
            DEFER(Filesystem::unlock());
            auto entry = Filesystem::unsafeGetEntryNode(path);
            for (auto element : entry->children) {
                if (!Filesystem::isHiddenFile(element.first) && element.second->status) {
                    struct stat* status = element.second->status;
                    char time_str[100];
                    char mode_str[11];

                    nblocks += status->st_blocks / 2;

                    timeToStr(status->st_mtime, time_str, 100);

                    strmode(status->st_mode, mode_str);

                    quotingExists |= needsQuoting(element.first);

                    Lsdata filedata = {
                                       std::string(mode_str),
                                       std::string(time_str),
                                       Filesystem::unsafeGetGroup(status->st_uid),
                                       Filesystem::unsafeGetUser(status->st_uid),
                                       element.first,
                                       std::to_string(status->st_nlink),
                                       std::to_string(status->st_size)
                    };

                    length.update(filedata);

                    files.push_back(filedata);
                }
            }
        }

        /* Then, every file can be sorted according to its name.
           `strcoll` is a local-dependant string comparison.
        */
        std::sort(files.begin(), files.end(),
                  [](const Lsdata &a, const Lsdata &b) {
                      return strcoll(a.name.c_str(), b.name.c_str()) < 0;
                  });


        /* Finally, we can output everything */
        Lsformatstr formatstr(length);
        char c_str[1024];
        socket << "total " << std::to_string(nblocks) << "\n";
        for (auto lsdata : files) {
            // use quotingExists to be more compatible with the shell version
            // when it's executed from a fancy shell
            formatstr.dump(lsdata, 1024, c_str, false/*quotingExists*/);
            // already includes the endline
            socket << c_str;
        }
    }


    void execute(Socket &socket, Context &context, const CommandArgs &) {

        if (true) {
            my_ls(context.getAbsolutePath(), socket);
        } else {
            //TODO add sanitization
            socket << exec(std::string("ls -l ")
                           + context.getAbsolutePath().string()) << "\n";
        }
    }

    Specification getSpecification() const {
        return {};
    }
private:
};
std::array<std::string, 12 * 2> LsCommand::date_fmt =
    {
     "Jan %e  %Y",
     "Feb %e  %Y",
     "Mar %e  %Y",
     "Apr %e  %Y",
     "May %e  %Y",
     "Jun %e  %Y",
     "Jul %e  %Y",
     "Aug %e  %Y",
     "Sep %e  %Y",
     "Oct %e  %Y",
     "Nov %e  %Y",
     "Dec %e  %Y",
     "Jan %e %H:%M",
     "Feb %e %H:%M",
     "Mar %e %H:%M",
     "Apr %e %H:%M",
     "May %e %H:%M",
     "Jun %e %H:%M",
     "Jul %e %H:%M",
     "Aug %e %H:%M",
     "Sep %e %H:%M",
     "Oct %e %H:%M",
     "Nov %e %H:%M",
     "Dec %e %H:%M"
    };


void timeToStr(time_t time, char* time_str, unsigned int n) {
    struct timespec when_timespec;
    struct timespec current_time;
    struct timespec six_months_ago;
    struct tm *when_local;

    when_timespec.tv_sec = time;
    when_timespec.tv_nsec = 0;
    when_local =localtime(&time);
    gettime(&current_time);

    six_months_ago.tv_sec = current_time.tv_sec - 31556952 / 2;
    six_months_ago.tv_nsec = current_time.tv_nsec;

    bool recent = (timespec_cmp (six_months_ago, when_timespec) < 0
                   && (timespec_cmp (when_timespec, current_time) < 0));
    strftime(time_str, n, LsCommand::date_fmt[12 * recent + when_local->tm_mon].c_str(), when_local);
}


REGISTER_COMMAND(LsCommand, "ls");

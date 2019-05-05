<pre style="background: white">
 .d88b.   8888b.  88888888  .d88b.  88888b.
d88P"88b     "88b    d88P  d88""88b 888 "88b
888  888 .d888888   d88P   888  888 888  888
Y88b 888 888  888  d88P    Y88..88P 888  888
 "Y88888 "Y888888 88888888  "Y88P"  888  888
     888
Y8b d88P      <b>GRASS - Grep as a Service</b>
 "Y88P"       <i>Sofware Security (CS-412)</i>
</pre>

[![Build Status](https://travis-ci.com/poechsel/gazon.svg?token=G2RPssxKyYp3kDV9Yo3a&branch=master)](https://travis-ci.com/poechsel/gazon)

Pierre Oechsel: 294718  
Romain Liautaud: 298634  

## About this project.

_Gazon_ is our implementation of the _GRASS_ protocol, a simple file transfer protocol designed as an excuse to apply engineering practices from the CS-412 course at EPFL. It is implemented in C++11, and requires no external dependencies.

- **Run `make` to build the project**.
- Run `bin/server` to start a _Gazon_ server. It will look for the `grass.conf` file in the current directory.
- Run `bin/client host port [infile outfile]` to start a _Gazon_ client and connect it to the server at a given host and port. If `infile` and `outfile` arguments are specified, the client will start in automated testing mode: it will read the commands from the infile, send them sequentially, and write the server responses to the outfile.

_Note: we squashed our entire Git history to make the exploit phase a little more interesting. If this is a problem, we kept the history on a private fork, so you can just request it by email to pierre.oechsel or romain.liautaud._

## Integration tests.

We have implemented some integration tests, which mostly mimic the tests provided in the assignment. They check both the correctness of the commands and the formatting of their output.

To run these tests, you will need `pytest` (which can be installed with `pip3`). Then, simply run `python3 -m pytest`.

## Available commands.

### For non-logged users.

- `login $username` followed by `pass $password` tries to log the user in with the given credentials.
- `ping $hostname` runs the `ping` command on the server and return the results.
- `exit` closes the current session.

### For logged users.

- `grep $pattern` checks all the files in the current server directory (and its subdirectories) for the given extended pattern.
- `get $path` retrieves a file from the server.
- `put $path $size` uploads a file to the server.
- `logout` disconnects the current logged user.
- `ls` prints out the contents of the current server directory.
- `cd $path` changes the current server directory.
- `mkdir $path` creates a new directory on the server.
- `rm $path` removes the file at the given path from the server.
- `w` returns the alphabetically sorted list of sorted users.
- `whoami` returns the username of the current logged user.
- `date` prints the current date and time.

**Note that we support running multiple `get` and `put` commands in parallel.** To achieve this without ambiguity, we've had to change the protocol slightly (which shouldn't be an issue since we need not be interoperable anymore). Now, the `get` command sends back `get port: $port size: $size path: $path` to the client, and the `put` command sends back `put port: $port path: $path`. This is used to keep track of which port corresponds to which file.

## Project architecture.

This project is split into several folders:
- `include` contains all the C++ header files;
- `src` contains all the C++ source files;
- `bin` will receive the binaries and compilation artifacts;
- `tests` contains a series of integration tests written in Python.

Since we will to compile two separate binaries (one for the client and one for the server), both the `include` and `src` folders are split into three subfolders: `client`, `server` and `common`. The `common` folder contains all the code to be shared between the two binaries (e.g. the `Command` or `Socket` abstractions), some of which will be detailed below.

We implemented a thread pool abstraction (see `include/common/threadpool.h`) to schedule jobs without the overhead of creating a new OS thread each time. Almost every task in our implementation of GRASS is scheduled onto a thread pool, from the dispatch and execution of commands to the file transfer mechanism used by `get` and `put`.

We also implemented a command abstraction (see `include/common/command.h`) which handles everything from registering commands to type-checking, casting and sanitizing the raw arguments passed to the different GRASS commands. To summarize, every GRASS command implemented in `src/commands` inherits from the `Command` abstract class, and declares both an `execute` function, a specification for the types of the arguments, and a middleware (which is responsible for checking the current session before a command executes, e.g. to ensure that the user is logged in).

To facilitate the communication between the client and the server, we implemented two abstractions in `include/common/socket.h`: an overlay over TCP sockets and a connection pool. The `Socket` overlay helps read and write newline-separated packets, while the `ConnectionPool` takes care of cleaning up closed sockets, as well as `select()`-ing sockets which are ready to be read from.

To improve performance and clarify the parts of our code which interact with the file system, we implemented a virtual filesystem abstraction in `include/common/filesystem.h` and `include/common/file.h`. It caches the current contents of the base directory and its subdirectories, and gives a cleaner (and thread-safe) API to manipulate files.

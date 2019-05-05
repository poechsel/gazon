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


## About this project.

_Gazon_ is our implementation of the _GRASS_ protocol, a simple file transfer protocol designed as an excuse to apply engineering practices from the CS-412 course at EPFL. It is implemented in C++11, and requires no external dependencies.

- **Use `make` to build the project**.
- Use `bin/server` to start a _Gazon_ server. It will look for the `grass.conf` file in the current directory.
- Use `bin/client host port [infile outfile]` to start a _Gazon_ client and connect it to the server at a given host and port. If `infile` and `outfile` arguments are specified, the client will start in automated testing mode: it will read the commands from the infile, send them sequentially, and write the server responses to the outfile.

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
import pytest
from shutil import copyfile
import subprocess

def proc_still_running(proc):
    return proc is not None and proc.poll() is None

def stop_proc(proc):
    if proc_still_running(proc):
        proc.terminate()

def launch_proc(args, waitfor = None):
    proc = subprocess.Popen(args,
                                  stdout = subprocess.PIPE)
    if waitfor is None:
        return proc

    for line in iter(proc.stdout.readline, b''):
        if waitfor in line:
            break
    return proc

def generate_grass_conf(basedir, users):
    users_str = ""
    for (user, pwd) in users.items():
        users_str += "user {} {}\n".format(user, pwd)
    return """
    base {}
    port 1337
    {}
    """.format(basedir, users_str)

@pytest.fixture(scope="function")
def server(request):
    def server_end():
        stop_proc(server)
        copyfile("grass.back", "grass.conf")

    copyfile("grass.conf", "grass.back")

    basedir = request.node.get_closest_marker("basedir").args[0]
    users = request.node.get_closest_marker("users").args[0]
    grass_conf_content = generate_grass_conf(basedir, users)
    with open("grass.conf", "w") as grass_conf:
        grass_conf.write(grass_conf_content)

    server = launch_proc(["./bin/server"])

    yield server

    request.addfinalizer(server_end)

import pytest
from shutil import copyfile
from tempfile import mkdtemp
import subprocess
import re, os
from time import sleep

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

fs_write_line = re.compile("(.*?): write \"(.*?)\"", re.MULTILINE)
fs_len_line = re.compile("(.*?): len (.*)")

def load_fs(basedir, modelname):
    with open(modelname, 'r') as model:
        for line in model.readlines():
            result_write = fs_write_line.match(line)
            result_len = fs_len_line.match(line)
            if result_len is None and result_write is None:
                print("incorrect line for fs: ", line)
                continue
            filename = ""
            filecontent = ""
            if not result_len is None:
                filename = result_len.group(1)
                filelen = int(result_len.group(2))
                filecontent = "a" * filelen
            else:
                filename = result_write.group(1)
                filecontent = result_write.group(2)

            filename = basedir + "/" + filename
            os.makedirs(os.path.dirname(filename), exist_ok=True)
            with open(filename, "w") as f:
                f.write(filecontent)


@pytest.fixture(scope="function")
def server(request):
    def server_end():
        stop_proc(server)
        copyfile("grass.back", "grass.conf")

    copyfile("grass.conf", "grass.back")
    copyfile("grass.conf", "grass.back02")

    basedir = mkdtemp()
    users = request.node.get_closest_marker("users").args[0]

    fs_marker = request.node.get_closest_marker("fs")
    if fs_marker is not None:
        load_fs(basedir, fs_marker.args[0])

    client_fs_marker = request.node.get_closest_marker("clientfs")
    if client_fs_marker is not None:
        load_fs(".", fs_marker.args[0])

    grass_conf_content = generate_grass_conf(basedir, users)
    with open("grass.conf", "w") as grass_conf:
        grass_conf.write(grass_conf_content)

    server = launch_proc(["./bin/server"])
    sleep(.5)
    if fs_marker is not None:
        sleep(1)

    yield server
    request.addfinalizer(server_end)

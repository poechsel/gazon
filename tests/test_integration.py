from tests.common import *
from tempfile import mktemp
import time
import re

to_ignore_pattern = "<i>"
def compare_line(line, truth):
    if "<line>" in truth:
        return True
    # print("comparing: ", line, truth)
    truth_parts = truth.split("<*>")
    truth_parts = [re.escape(part) for part in truth_parts]
    regex = "^" + ".*".join(truth_parts) + "$"
    return re.match(regex, line) is not None

def boilerplate(confs):
    if isinstance(confs, str):
        confs = [confs]
    confs = [(x, 0) if not isinstance(x, tuple) else x for x in confs]

    instances = []
    for (conf, delay) in confs:
        temp = mktemp()
        confin = "{}.in".format(conf)
        confout = "{}.out".format(conf)
        args = ["./bin/client", "127.0.0.1", "1337", confin, temp]
        if delay > 0:
            time.sleep(delay)
        client = launch_proc(args)
        instances.append((client, temp, confout))

    time.sleep(3)

    for (client, _, _) in instances:
        stop_proc(client)

    for (_, temp, confout) in instances:
        templines = []
        with open(temp, 'r') as tempfile:
            templines = tempfile.readlines()
            templines = [line for line in templines if len(line.strip())]
        outlines = []
        with open(confout, 'r') as outfile:
            outlines = outfile.readlines()
            outlines = [line for line in outlines if len(line.strip())]

        assert(len(templines) == len(outlines))
        for line_temp, line_out in zip(templines, outlines):
            assert(compare_line(line_temp, line_out))


@pytest.mark.users({"u1" : "p1"})
def test_ping(server):
    boilerplate("tests/ping/ping")

@pytest.mark.users({"u1" : "p1"})
def test_directories(server):
    boilerplate("tests/directories/directory")

@pytest.mark.users({"u1" : "p1", "u2": "p2", "u3": "p3"})
def test_mutliclients(server):
    boilerplate([("tests/multiclients/client1", 0),
                 ("tests/mutliclients/client2", 1),
                 ("tests/mutliclients/client2", 2)])

@pytest.mark.users({"u1" : "p1"})
@pytest.mark.fs("tests/grep/fs.model")
def test_grep(server):
    boilerplate("tests/grep/grep")

@pytest.mark.users({"u1" : "p1"})
@pytest.mark.fs("tests/performance/fs.model")
def test_perf(server):
    boilerplate("tests/performance/grep")

@pytest.mark.users({"u1" : "p1"})
@pytest.mark.fs("tests/performance/fs_small.model")
def test_perf_small(server):
    boilerplate("tests/performance/grep_small")

@pytest.mark.users({"u1" : "p1"})
@pytest.mark.fs("tests/grep2/fs.model")
def test_grep2(server):
    boilerplate("tests/grep2/grep2")

@pytest.mark.users({"u1" : "p1"})
@pytest.mark.fs("tests/transfer/fs.model")
@pytest.mark.clientfs("tests/transfer/clientfs.model")
def test_transfer(server):
    boilerplate("tests/transfer/transfer")

@pytest.mark.users({"u1" : "p1", "u2" : "p2"})
def test_authentication(server):
    boilerplate("tests/authentication/auth")

@pytest.mark.users({"u1" : "p1"})
@pytest.mark.fs("tests/errors/fs.model")
def test_errors(server):
    boilerplate("tests/errors/errors")

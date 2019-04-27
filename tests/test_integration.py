from tests.common import *
from tempfile import mktemp
import time

def boilerplate(confs):
    if isinstance(confs, str):
        confs = [confs]
    instances = []
    for conf in confs:
        temp = mktemp()
        confin = "{}.in".format(conf)
        confout = "{}.out".format(conf)
        args = ["./bin/client", "127.0.0.1", "1337", confin, temp]
        client = launch_proc(args)
        instances.append((client, temp, confout))

    time.sleep(5)

    test_success = True
    for (client, temp, confout) in instances:
        stop_proc(client)
        if not test_success:
            continue
        templines = []
        with open(temp, 'r') as tempfile:
            templines = tempfile.readlines()
        outlines = []
        with open(confout, 'r') as outfile:
            outlines = outfile.readlines()

        if len(templines) != len(outlines):
            test_success = False
        else:
            for line_temp, line_out in zip(templines, outlines):
                if "CANVARY" in line_out:
                    continue
                test_success &= line_temp == line_out
    return test_success

@pytest.mark.users({"u1" : "p1"})
@pytest.mark.basedir("test01")
def test_ping(server):
    boilerplate("tests/ping")

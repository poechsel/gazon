def gen_grep_results(name, n):
    MN = len(str(n - 1))
    with open(name, 'w') as f:
        for i in range(1, n):
            s = str(i).zfill(MN)
            if "7" in s:
                f.write("File{}.txt\n".format(s))

def gen_grep_fs(name, n):
    MN = len(str(n - 1))
    with open(name, 'w') as f:
        for i in range(1, n):
            s = str(i).zfill(MN)
            f.write("File{}.txt: write \"This is file {}.\"\n".format(s, s))

gen_grep_fs("tests/performance/fs.model", 10000)
gen_grep_results("tests/performance/grep.out", 10000)
gen_grep_fs("tests/performance/fs_small.model", 99)
gen_grep_results("tests/performance/grep_small.out", 99)

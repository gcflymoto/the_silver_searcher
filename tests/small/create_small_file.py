#!/usr/bin/env python

# Create an 2.2GB file of mostly "abcdefghijklmnopqrstuvwxyz01234",
# with a few instances of "hello"

import sys

if len(sys.argv) != 2:
    print("Usage: %s small_file.txt" % sys.argv[0])
    sys.exit(1)

small_file = sys.argv[1]


def create_small_file():
    with open(small_file, "w") as fd:
        for i in range(1, 2**26):
            byte = i * 32
            if byte % 2**28 == 0:
                fd.write("hello%s\n" % byte)
            else:
                fd.write("abcdefghijklmnopqrstuvwxyz01234\n")
        fd.write("hello\n")


try:
    fd = open(small_file, "r")
except Exception as e:
    create_small_file()

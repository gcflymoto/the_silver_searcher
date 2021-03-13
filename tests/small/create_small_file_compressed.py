#!/usr/bin/env python

# Create an 2.2GB compressed file of mostly "abcdefghijklmnopqrstuvwxyz01234",
# with a few instances of "hello"

import os
import sys

if len(sys.argv) != 2:
    print("Usage: %s small_file.txt" % sys.argv[0])
    sys.exit(1)

small_file     = sys.argv[1]
small_file_gz  = small_file + '.gz'
small_file_bz2 = small_file + '.bz2'
small_file_xz  = small_file + '.xz'

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

try:
    fd = open(small_file_gz)
except Exception as e:
    os.system("gzip < " + small_file + " > " + small_file_gz)

try:
    fd = open(small_file_bz2)
except Exception as e:
    os.system("bzip2 < " + small_file + " > " + small_file_bz2)

try:
    fd = open(small_file_xz)
except Exception as e:
    os.system("xz < " + small_file + " > " + small_file_xz)

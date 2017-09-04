#! /usr/bin/env python3

import sys
import difflib

if len(sys.argv) != 3:
    sys.exit(2)

try:
    with open(sys.argv[1]) as infile1:
        first = infile1.readlines()
except UnicodeDecodeError:
    with open(sys.argv[1], encoding="utf_16") as infile1:
        first = infile1.readlines()
try:
    with open(sys.argv[2]) as infile2:
        second = infile2.readlines()
except UnicodeDecodeError:
    with open(sys.argv[2], encoding="utf_16") as infile2:
        second = infile2.readlines()

diffs = list(difflib.unified_diff(first, second,
                                  fromfile=sys.argv[1], tofile=sys.argv[2]))
if diffs:
    sys.stdout.writelines(diffs)
    sys.exit(1)

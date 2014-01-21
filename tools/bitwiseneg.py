#!/usr/bin/env python
import sys

for c in sys.argv[1]:
    if c == '0':
        print(1,end="")
    else:
        print(0,end="")
print()

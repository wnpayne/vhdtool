#!/usr/bin/env python
import sys

looplen = 32 - len(sys.argv[1])

for c in range(0,looplen):
    print(0,end="")

print(sys.argv[1])

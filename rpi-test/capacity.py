#!/usr/bin/env python

# calculate channel max bitrate given modulation settings
# converted from the equivalent calculation in tsrfsend

import sys
from fractions import Fraction

def usage():
  print "Usage: "+sys.argv[0]+" bandwidth qam fec gi"
  print "     bandwidth(kHz): 2000 to 8000"
  print "     qam: 4 | 16 | 64 (QPSK,16QAM,64QAM)"
  print "     fec: 1/2 | 2/3 | 3/4 | 5/6 | 7/8"
  print "     gi:  1/4 | 1/8 | 1/16 | 1/32"
  sys.exit(1)

if len(sys.argv) != 5:
  usage()

# bandwidth
capacity = Fraction(1000,1) * int(sys.argv[1])

# constellation
try:
  capacity *= {
    '4': 2,
    '16': 4,
    '64': 6,
  }[sys.argv[2]]
except KeyError:
  print "ERROR: Invalid constellation\n"
  usage()

# fec
try:
  capacity *= {
    '1/2': Fraction(1,2),
    '2/3': Fraction(2,3),
    '3/4': Fraction(3,4),
    '5/6': Fraction(5,6),
    '7/8': Fraction(7,8),
  }[sys.argv[3]]
except KeyError:
  print "ERROR: Invalid fec\n"
  usage()

# guard interval
try:
  capacity *= {
    '1/4': Fraction(4,5),
    '1/8': Fraction(8,9),
    '1/16': Fraction(16,17),
    '1/32': Fraction(32,33),
  }[sys.argv[4]]
except KeyError:
  print "ERROR: Invalid guard interval\n"
  usage()

capacity = int(capacity)

capacity /= Fraction(544,423)

print int(capacity)


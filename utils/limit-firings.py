#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: limit-firings.py FILENAME START_ID END_ID

Limit the somas in a firings file to those with IDs within a given range.

Example: limit-firings.py 0_of_1_firings_20140529114219.txt 2012474 2022473
This outputs limited_0_of_1_firings_20140529114219.txt, a firings file
including only the somas with IDs #2012474 to #2022473.
"""

from __future__ import print_function, with_statement

import sys
import os.path

if len(sys.argv) < 4:
	print(__doc__[1:-1])
	sys.exit(1)

# Prepend "limited_" to the input filename for the output file
filepath = sys.argv[1]
dirname, filename = os.path.split(filepath)
new_filepath = os.path.join(dirname, 'limited_' + filename)

start_id = int(sys.argv[2])
end_id = int(sys.argv[3])
valid_ids = set(range(start_id, end_id+1))

# Limit firing somas to the desired IDs
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	# Copy first 9 lines
	# Example:
	#     # CRC for input file = 2947219980, excludes first line and line endings.
	#     v 2
	#     # Next line contains the time step in microseconds (100 - 10000):
	#     500
	#     # Next line contains the total soma count:
	#     2423776
	#     # Next line contains the number of cycles:
	#     2000
	#     # Cycle number <space> Number of SomaIds <space> SomaId1 <space> SomaId1_State <space> SomaId2 <space> SomaId2_State <space> ... SomaIdN <space> SomaIdN_State
	for _ in range(9):
		new_f.write(f.readline())
	
	# Limit firing somas to the desired IDs
	for line in f:
		tokens = line.split()
		cycle, num_all, pairs = tokens[0], tokens[1], tokens[2:]
		pairs = [' '.join(p) for p in zip(*(iter(pairs),)*2) if int(p[0]) in valid_ids]
		new_f.write('%s %s %s\n' % (cycle, str(len(pairs)), ' '.join(pairs)))

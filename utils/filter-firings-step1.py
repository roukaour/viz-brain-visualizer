#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: filter-firings-step1.py FILENAME

Transposes a firings file to have one (cycle, soma) pair per line.
Lines are tab-separated values with the column headers:
Cycle#	#All	SomaID	FiringType

Example: filter-firings.py 0_of_1_firings_20140529114219.txt
This outputs transposed_forced_fire_rows_0_of_1_firings_20140529114219.txt.
"""

from __future__ import print_function, with_statement

import sys
import os
import os.path

if len(sys.argv) < 2:
	print(__doc__[1:-1])
	sys.exit(1)

# Prepend "transposed_forced_fire_rows_" to the input filename for the output file
filepath = sys.argv[1]
dirname, filename = os.path.split(filepath)
new_filepath = os.path.join(dirname, 'transposed_forced_fire_rows_' + filename)

# Transpose rows so that each firing soma is on its own row
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	# Replace the first 9 lines with column headers
	# Example:
	#     # CRC for input file = 682219879, excludes first line and line endings.
	#     v 2
	#     # Next line contains the time step in microseconds (100 - 10000):
	#     500
	#     # Next line contains the total soma count:
	#     24876
	#     # Next line contains the number of cycles:
	#     10000
	#     # Cycle number <space> Number of SomaIds <space> SomaId1 <space> SomaId1_State <space> SomaId2 <space> SomaId2_State <space> ... SomaIdN <space> SomaIdN_State
	# becomes:
	#     Cycle#	#All	SomaID	FiringType
	for _ in range(9):
		f.readline()
	new_f.write('Cycle#\t#All\tSomaID\tFiringType\n')
	
	# Transpose rows so that each firing soma is on its own row
	# Example:
	#     387 2 5 1 10 2 
	# becomes:
	#     387	2	5	1
	#     387	2	10	2
	line = True
	while line:
		line = f.readline()
		tokens = line.split()
		prefix, pairs = tokens[:2], tokens[2:]
		while pairs:
			pair, pairs = pairs[:2], pairs[2:]
			new_f.write('\t'.join(prefix + pair) + '\n')

os.utime(__file__, None) # touch filter-firings-step1.py

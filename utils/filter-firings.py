#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: filter-firings.py FILENAME

Transposes a firings file to have one (cycle, firing type) pair per line.
Lines are tab-separated values with the column headers:
Cycle#	#All	#Fired	TypeFired	SomaIDs

Example: filter-firings.py 0_of_1_firings_20140529114219.txt
This outputs retransposed_forced_fire_rows_0_of_1_firings_20140529114219.tsv.
"""

from __future__ import print_function, with_statement

import sys
import os
import os.path

if len(sys.argv) < 2:
	print(__doc__[1:-1])
	sys.exit(1)

# Prepend "retransposed_forced_fire_rows_" to the input filename and replace extension with ".tsv" for the output file
filepath = sys.argv[1]
dirname, filename = os.path.split(filepath)
filename_noext = os.path.splitext(filename)[0]
new_filepath = os.path.join(dirname, 'retransposed_forced_fire_rows_' + filename_noext + '.tsv')

# Transpose rows so that each (cycle, firing type) pair is on its own row
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	# Skip first 9 lines
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
	for _ in range(9):
		f.readline()
	
	# Output the column headers
	# Example:
	#     Cycle#	#All	#Fired	TypeFired	SomaIDs
	new_f.write('Cycle\t#All\t#Fired\tTypeFired\tSomaIDs\n')
	
	# Transpose rows so that each (cycle, firing type) pair is on its own row
	# Example:
	#     387 4 5 1 6 1 7 1 10 2 
	# becomes:
	#     387	4	3	1	5	6	7
	#     387	4	1	2	10
	for line in f:
		tokens = line.split()
		cycle, num_all, pairs = tokens[0], tokens[1], tokens[2:]
		data = {} # {int(firing type): [soma IDs]}
		while pairs:
			soma_id, firing_type, pairs = pairs[0], int(pairs[1]), pairs[2:]
			if firing_type not in data:
				data[firing_type] = []
			data[firing_type].append(soma_id)
		for firing_type in sorted(data.keys()):
			soma_ids = data[firing_type]
			new_f.write('\t'.join([cycle, num_all] + [str(len(soma_ids))] + [str(firing_type)] + soma_ids) + '\n')

os.utime(__file__, None) # touch filter-firings.py

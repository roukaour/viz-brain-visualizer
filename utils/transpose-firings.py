#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: transpose-firings.py FILENAME

Transposes a firings file to have one soma ID per line, followed by their
firing cycles and types.
Note that this script needs around twice as much memory as the firings file,
so a 100MB firings file will require 200MB of memory to transpose.

Example: transpose-firings.py 0_of_1_firings_20140529114219.txt
This outputs transposed_0_of_1_firings_20140529114219.txt.
"""

from __future__ import print_function, with_statement

import sys
import os
import os.path

if len(sys.argv) < 2:
	print(__doc__[1:-1])
	sys.exit(1)

# Prepend "transposed_" to the input filename for the output file
filepath = sys.argv[1]
dirname, filename = os.path.split(filepath)
new_filepath = os.path.join(dirname, 'transposed_' + filename)

soma_to_cycle_and_state_pair_map = {}

# Transpose cycle lines of soma IDs to soma ID lines of cycles
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	# Copy first 8 lines
	# Example:
	#     # CRC for input file = 2947219980, excludes first line and line endings.
	#     v 2
	#     # Next line contains the time step in microseconds (100 - 10000):
	#     500
	#     # Next line contains the total soma count:
	#     2423776
	#     # Next line contains the number of cycles:
	#     2000
	for _ in range(8):
		new_f.write(f.readline())
	
	# Revise the description line
	# Example:
	#     # Cycle number <space> Number of SomaIds <space> SomaId1 <space> SomaId1_State <space> SomaId2 <space> SomaId2_State <space> ... SomaIdN <space> SomaIdN_State
	# becomes:
	#     # SomaId <space> Number of Cycles <space> Cycle1 <space> State1 <space> Cycle2 <space> State2 <space> ... CycleN <space> StateN
	f.readline()
	new_f.write('# SomaId <space> Number of Cycles <space> Cycle1 <space> State1 <space> Cycle2 <space> State2 <space> ... CycleN <space> StateN\n')
	
	# Gather data
	for line in f:
		tokens = line.split()
		cycle, num_all, pairs = int(tokens[0]), tokens[1], zip(*(iter(tokens[2:]),) * 2)
		for pair in pairs:
			soma_id, firing_state = int(pair[0]), int(pair[1])
			if soma_id not in soma_to_cycle_and_state_pair_map:
				soma_to_cycle_and_state_pair_map[soma_id] = []
			soma_to_cycle_and_state_pair_map[soma_id].extend([cycle, firing_state])
	
	# Write data
	for soma_id in sorted(soma_to_cycle_and_state_pair_map.keys()):
		pairs = soma_to_cycle_and_state_pair_map[soma_id]
		new_f.write('%d %d %s\n' % (soma_id, len(pairs) / 2, ' '.join(map(str, pairs))))

os.utime(__file__, None) # touch transpose-firings.py

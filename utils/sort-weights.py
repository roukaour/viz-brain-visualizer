#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: sort-weights.py FILENAME

Sorts a weights file by (Cycle, RunSimSynapseID) in ascending order.

Example: sort-weights.py 0_of_1_weights_20140601163509.txt
This outputs sorted_0_of_1_weights_20140601163509.txt.
"""

from __future__ import print_function, with_statement

import sys
import os
import os.path

if len(sys.argv) < 2:
	print(__doc__[1:-1])
	sys.exit(1)

# Prepend "sorted_" to the input filename for the output file
filepath = sys.argv[1]
dirname, filename = os.path.split(filepath)
new_filepath = os.path.join(dirname, 'sorted_' + filename)

# Sort weights by (Cycle, RunSimSynapseID) in ascending order
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	# Copy first 8 lines
	# Example:
	#     # Next line contains the total soma count (includes inactive):
	#     217648
	#     # Next line contains the total synapse count:
	#     9159597
	#     # Next line contains the number of cycles:
	#     5000
	#     #Cycle number <space> Init Synapse Id <space> Axonal soma ID <space> Dendritic soma ID <space> Runsim Synapse Id <space> Weight before <space> Weight after
	#     #Cycle InitSynapseId AxonalSoma DendriticSoma RunSimSynapseId WeightBefore WeightAfter
	for _ in range(8):
		new_f.write(f.readline())
	
	# Sort consecutive weights with the same cycle by their RunSimSynapseID
	line = True
	cycle = None
	pairs = []
	while line:
		line = f.readline()
		tokens = line.split()
		if not tokens or tokens[0] != cycle:
			# Write data from previous cycle
			for pair in sorted(pairs):
				new_f.write(pair[1])
			# Start data for current cycle
			if tokens:
				cycle = tokens[0]
				pairs = []
		# Add data for current cycle
		if tokens:
			pairs.append((int(tokens[4]), line))

os.utime(__file__, None) # touch sort-weights.py

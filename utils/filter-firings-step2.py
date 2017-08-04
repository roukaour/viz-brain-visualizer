#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: filter-firings-step2.py FILENAME

Retransposes a transposed firings file (the output of filter-firings-step1.py)
to have one (cycle, firing type) pair per line.
Lines are tab-separated values with the column headers:
Cycle#	#All	#Fired	TypeFired	SomaIDs

Example: filter-firings.py transposed_forced_fire_rows_0_of_1_firings_20140529114219.txt
This outputs retransposed_forced_fire_rows_0_of_1_firings_20140529114219.tsv.
"""

from __future__ import print_function, with_statement

import sys
import os
import os.path

if len(sys.argv) < 2:
	print(__doc__[1:-1])
	sys.exit(1)

# Prepend "re" to the input filename and replace extension with ".tsv" for the output file
filepath = sys.argv[1]
dirname, filename = os.path.split(filepath)
filename_noext = os.path.splitext(filename)[0]
new_filepath = os.path.join(dirname, 're' + filename_noext + '.tsv')

# Retranspose rows so that each (cycle, firing type) pair is on its own row
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	# Revise the column header line
	# Example:
	#     Cycle#	#All	SomaID	FiringType
	# becomes:
	#     Cycle#	#All	#Fired	TypeFired	SomaIDs
	f.readline()
	new_f.write('Cycle\t#All\t#Fired\tTypeFired\tSomaIDs\n')
	
	# Retranspose rows so that each firing type is on its own row
	# Example:
	#     387	4	5	1
	#     387	4	6	1
	#     387	4	10	2
	# becomes:
	#     387	4	2	1	5	6
	#     387	4	1	2	10
	line = True
	cycle = None
	num_all = None
	data = {} # {int(firing type): [soma IDs]}
	while line:
		line = f.readline()
		tokens = line.split()
		if not tokens or tokens[0] != cycle:
			# Write data from previous cycle
			for firing_type in sorted(data.keys()):
				soma_ids = data[firing_type]
				new_f.write('\t'.join([cycle, num_all] + [str(len(soma_ids))] + [str(firing_type)] + soma_ids) + '\n')
			# Start data for current cycle
			if tokens:
				cycle = tokens[0]
				num_all = tokens[1]
				data = {}
		# Add data for current cycle
		if tokens:
			soma_id, firing_type = tokens[2], int(tokens[3])
			if firing_type not in data:
				data[firing_type] = []
			data[firing_type].append(soma_id)

os.utime(__file__, None) # touch filter-firings-step2.py

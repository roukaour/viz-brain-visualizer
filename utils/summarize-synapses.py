#!/usr/bin/env python
# Works with Python 2.7 or 3.3

"""
Usage: summarize-synapses.py FILENAME AXON DEN [MIN MAX WHICH [LIMIT=100]]

Prints statistics about the A-to-D synapse counts for all A somas and D somas
in a text model file, for given soma types A and D.
If MIN, MAX, and WHICH are specified, prints a list of the WHICH-type somas
with counts between MIN and MAX. At most LIMIT number of somas are printed
(or no limit if LIMIT=0).
Note that binary models must be converted to text with convert-model.py.

Example: summarize-synapses.py viz_input.txt N P
This outputs histograms of the N-to-P synapse counts for N and P somas.

Example: summarize-synapses.py viz_input.txt M R 0 0 M 0
This lists all of the M somas without R children.
"""

from __future__ import print_function, with_statement, division

import sys
from collections import defaultdict

try:
	xrange
except NameError:
	xrange = range

def input_file_lines(filename):
	with open(filename, 'r') as input_file:
		for line in input_file:
			line = line.split('#', 2)[0].strip()
			if line:
				yield line

if len(sys.argv) < 4:
	print(__doc__[1:-1])
	exit(1)

filename = sys.argv[1]
track_axon_letter = sys.argv[2].upper()
track_den_letter = sys.argv[3].upper()
minimum = int(sys.argv[4]) if len(sys.argv) > 6 else None
maximum = int(sys.argv[5]) if len(sys.argv) > 6 else None
track_which_letter = sys.argv[6].upper() if len(sys.argv) > 6 else None
count_limit = int(sys.argv[7]) if len(sys.argv) > 7 else 100
lines = input_file_lines(filename)

# Print the filename
print('Model file: {}'.format(filename))

# Store the type letter-to-ID mappings
types = {}
# Get the types
num_types = int(next(lines))
print('Getting {} types...'.format(num_types))
for index in xrange(num_types):
	# <id> <letter>
	line = next(lines).split()
	id = int(line[0])
	letter = line[1]
	
	types[letter] = id # map type letter to ID
print('Got {} types'.format(len(types)))

# Convert the requested synapse types from letters to IDs
track_axon_type_id = types[track_axon_letter]
track_den_type_id = types[track_den_letter]
print('Count all {}-to-{} ({}-to-{}) synapses'.format(track_axon_type_id, track_den_type_id, track_axon_letter, track_den_letter))

# Store the soma ID-to-count mappings
axon_somas = {}
den_somas = {}
axon_soma_count = 0
den_soma_count = 0
# Get the somas
num_somas = int(next(lines))
print('Getting {} somas...'.format(num_somas))
for index in xrange(num_somas):
	# <type_id> <soma_id> <x> <y> <z> <num_axon> <num_den>
	line = next(lines).split()
	type_id = int(line[0])
	soma_id = int(line[1])
	
	if type_id == track_axon_type_id:
		axon_somas[soma_id] = 0
		axon_soma_count += 1
	elif type_id == track_den_type_id:
		den_somas[soma_id] = 0
		den_soma_count += 1
	
	# Get the fields
	num_fields = int(line[5]) + int(line[6])
	for index2 in range(num_fields):
		# <x1> <x2> <y1> <y2> <z1> <z2>
		next(lines)
print('Got {} {} somas and {} {} somas'.format(axon_soma_count, track_axon_letter, den_soma_count, track_den_letter))

# Get the synapses
num_synapses = int(next(lines))
relevant_syn_count = 0
print('Getting {} synapses...'.format(num_synapses))
for index in xrange(num_synapses):
	# <id> <axon> <den> <x> <y> <z>
	# or
	# <id> 'v' <axon> <den> <vx> <vy> <vz> <x> <y> <z>
	line = next(lines).split()
	if line[1] == 'v':
		axon_id = int(line[2])
		den_id = int(line[3])
	else:
		axon_id = int(line[1])
		den_id = int(line[2])
	
	if axon_id in axon_somas and den_id in den_somas:
		axon_somas[axon_id] += 1
		den_somas[den_id] += 1
		relevant_syn_count += 1
print('Got {} {}-to-{} synapses'.format(relevant_syn_count, track_axon_letter, track_den_letter))

# Turn soma ID-to-count mapping into synapse count-to-soma count mapping
axon_syn_counts = defaultdict(lambda: 0)
den_syn_counts = defaultdict(lambda: 0)
max_axon_syn_count = 0
max_den_syn_count = 0
for soma_id in axon_somas:
	syn_count = axon_somas[soma_id]
	axon_syn_counts[syn_count] += 1
	if syn_count > max_axon_syn_count:
		max_axon_syn_count = syn_count
for soma_id in den_somas:
	syn_count = den_somas[soma_id]
	den_syn_counts[syn_count] += 1
	if syn_count > max_den_syn_count:
		max_den_syn_count = syn_count
print()

# Print histogram and summary statistics of axonal soma counts
print('{}-to-{} synapse counts for the {} {} somas:'.format(track_axon_letter, track_den_letter, axon_soma_count, track_axon_letter))
print('#{}to{}\t#{}\t%{}'.format(track_axon_letter, track_den_letter, track_axon_letter, track_axon_letter))
avg_axon_soma_syn_count = 0
for axon_syn_count in range(max_axon_syn_count + 1):
	soma_count = axon_syn_counts[axon_syn_count]
	avg_axon_soma_syn_count += axon_syn_count * soma_count
	soma_percent = float(soma_count) / axon_soma_count * 100
	print('{}\t{}\t{}%'.format(axon_syn_count, soma_count, soma_percent))
avg_axon_soma_syn_count /= axon_soma_count
print('Average synapse count is %.2f' % (avg_axon_soma_syn_count,))
print()

# Print histogram and summary statistics of dendritic soma counts
print('{}-to-{} synapse counts for the {} {} somas:'.format(track_axon_letter, track_den_letter, den_soma_count, track_den_letter))
print('#{}to{}\t#{}\t%{}'.format(track_axon_letter, track_den_letter, track_den_letter, track_den_letter))
avg_den_soma_syn_count = 0
for den_syn_count in range(max_den_syn_count + 1):
	soma_count = den_syn_counts[den_syn_count]
	avg_den_soma_syn_count += den_syn_count * soma_count
	soma_percent = float(soma_count) / den_soma_count * 100
	print('{}\t{}\t{}%'.format(den_syn_count, soma_count, soma_percent))
avg_den_soma_syn_count /= den_soma_count
print('Average synapse count is %.2f' % (avg_den_soma_syn_count,))

# Print somas with certain counts
if track_which_letter:
	print()
	print('{} somas with between {} and {} {}-to-{} synapses{}:'.format(track_which_letter, minimum, maximum, track_axon_letter, track_den_letter, ' (limit {})'.format(count_limit) if count_limit > 0 else ''))
	print('id\t#{}to{}'.format(track_axon_letter, track_den_letter))
	which_somas = axon_somas if track_which_letter == track_axon_letter else den_somas
	soma_count = 0
	for soma_id in which_somas:
		syn_count = which_somas[soma_id]
		if minimum <= syn_count <= maximum:
			print('{}\t{}'.format(soma_id, syn_count))
			soma_count += 1
			if count_limit > 0 and soma_count >= count_limit:
				print('LIMIT {} REACHED'.format(count_limit))
				break
	else:
		print('ALL {}'.format(soma_count))

#!/usr/bin/env python
# Works with Python 2.7 or 3.3

"""
Usage: summarize-synapses.py FILENAME AXON DEN [MIN MAX WHICH [LIMIT=100]]

Prints statistics about the A-to-D synapse counts for all A somas and D somas
in a binary model file, for given soma types A and D.
If MIN, MAX, and WHICH are specified, prints a list of the WHICH-type somas
with counts between MIN and MAX. At most LIMIT number of somas are printed
(or no limit if LIMIT=0).
Note that text models must be converted to binary with convert-model.py,
or use summarize-synapses.py for text models.

Example: summarize-synapses.py viz_input.vbm N P
This outputs histograms of the N-to-P synapse counts for N and P somas.

Example: summarize-synapses.py viz_input.vbm M R 0 0 M 0
This lists all of the M somas without R children.
"""

from __future__ import print_function, with_statement, division

import sys
import struct
from collections import defaultdict

try:
	xrange
except NameError:
	xrange = range

def read_cstring(f):
	chars = []
	c = f.read(1)
	while c != b'\0':
		chars.append(c)
		c = f.read(1)
	return b''.join(chars)

def read_byte(f):
	return struct.unpack('B', f.read(1))[0]

def read_unsigned(f):
	first = f.read(1)
	flag = struct.unpack('B', first)[0]
	if flag == 0xFF:
		rest = f.read(8)
		v = struct.unpack('>Q', rest)[0]
	elif (flag & 0xFE) == 0xFE:
		rest = f.read(7)
		v = struct.unpack('>Q', first + rest)[0] - 0xFE00000000000000
	elif (flag & 0xFC) == 0xFC:
		pad = b'\0'
		rest = f.read(6)
		v = struct.unpack('>Q', pad + first + rest)[0] - 0xFC000000000000
	elif (flag & 0xF8) == 0xF8:
		pad = b'\0\0'
		rest = f.read(5)
		v = struct.unpack('>Q', pad + first + rest)[0] - 0xF80000000000
	elif (flag & 0xF0) == 0xF0:
		pad = b'\0\0\0'
		rest = f.read(4)
		v = struct.unpack('>Q', pad + first + rest)[0] - 0xF000000000
	elif (flag & 0xE0) == 0xE0:
		rest = f.read(3)
		v = struct.unpack('>I', first + rest)[0] - 0xE0000000
	elif (flag & 0xC0) == 0xC0:
		pad = b'\0'
		rest = f.read(2)
		v = struct.unpack('>I', pad + first + rest)[0] - 0xC00000
	elif (flag & 0x80) == 0x80:
		rest = f.read(1)
		v = struct.unpack('>H', first + rest)[0] - 0x8000
	else:
		v = struct.unpack('B', first)[0]
	return v

def read_signed(f):
	first = f.read(1)
	flag = struct.unpack('B', first)[0]
	if flag == 0xFF:
		rest = f.read(4)
		v = struct.unpack('>I', rest)[0]
		if v & 0x80000000:
			v = -(v - 0x80000000)
	elif (flag & 0xFE) == 0xFE:
		rest = f.read(3)
		v = -(struct.unpack('>I', first + rest)[0] - 0xFE000000)
	elif (flag & 0xFC) == 0xFC:
		rest = f.read(3)
		v = struct.unpack('>I', first + rest)[0] - 0xFC000000
	elif (flag & 0xF8) == 0xF8:
		pad = b'\0'
		rest = f.read(2)
		v = -(struct.unpack('>I', pad + first + rest)[0] - 0xF80000)
	elif (flag & 0xF0) == 0xF0:
		pad = b'\0'
		rest = f.read(2)
		v = struct.unpack('>I', pad + first + rest)[0] - 0xF00000
	elif (flag & 0xE0) == 0xE0:
		rest = f.read(1)
		v = -(struct.unpack('>H', first + rest)[0] - 0xE000)
	elif (flag & 0xC0) == 0xC0:
		rest = f.read(1)
		v = struct.unpack('>H', first + rest)[0] - 0xC000
	elif (flag & 0x80) == 0x80:
		v = -(struct.unpack('B', first)[0] - 0x80)
	else:
		v = struct.unpack('B', first)[0]
	return v

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
file = open(filename, 'rb')

# Print the filename
print('Model file: {}'.format(filename))

# Ignore the model header
file.read(5) # file signature
file_version = read_byte(file)
read_cstring(file) # comment

# Store the type letter-to-ID mappings
types = {}
# Get the types
num_types = read_byte(file)
print('Getting {} types...'.format(num_types))
for index in xrange(num_types):
	# <letter>
	letter = file.read(1).decode('ascii')
	
	types[letter] = index # map type letter to ID
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
num_somas = read_unsigned(file)
if file_version >= 2:
	num_fields = read_unsigned(file)
print('Getting {} somas...'.format(num_somas))
for index in xrange(num_somas):
	# <type_id> <soma_id> <x> <y> <z> <num_axon> <num_den>
	type_id = read_byte(file)
	soma_id = read_unsigned(file)
	read_signed(file)
	read_signed(file)
	read_signed(file)
	num_axon = read_unsigned(file)
	num_den = read_unsigned(file)
	
	if type_id == track_axon_type_id:
		axon_somas[soma_id] = 0
		axon_soma_count += 1
	elif type_id == track_den_type_id:
		den_somas[soma_id] = 0
		den_soma_count += 1
	
	# Get the fields
	num_fields = num_axon + num_den
	for index2 in range(num_fields):
		# <x1> <x2> <y1> <y2> <z1> <z2>
		read_signed(file)
		read_signed(file)
		read_signed(file)
		read_signed(file)
		read_signed(file)
		read_signed(file)
print('Got {} {} somas and {} {} somas'.format(axon_soma_count, track_axon_letter, den_soma_count, track_den_letter))

# Get the synapses
num_synapses = read_unsigned(file)
relevant_syn_count = 0
print('Getting {} synapses...'.format(num_synapses))
for index in xrange(num_synapses):
	# <id> 0 <axon> <den> <x> <y> <z>
	# or
	# <id> 1 <axon> <den> <vx> <vy> <vz> <x> <y> <z>
	read_unsigned(file)
	has_via = read_byte(file)
	axon_id = read_unsigned(file)
	den_id = read_unsigned(file)
	if has_via:
		read_signed(file)
		read_signed(file)
		read_signed(file)
		read_signed(file)
		read_signed(file)
		read_signed(file)
	else:
		read_signed(file)
		read_signed(file)
		read_signed(file)
	
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

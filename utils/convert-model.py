#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: convert-model.py FILENAME

Converts a model file from binary to text, or vice-versa.

Example: convert-model.py 0_of_1_model_20140529114219.vbm
This outputs converted_text_0_of_1_model_20140529114219.txt.
"""

from __future__ import print_function, with_statement

import sys
import os
import os.path
import struct

BINARY_FILE_SIGNATURE = '\aRJV\xF7'
MAX_BINARY_FILE_FORMAT_VERSION = 2

def read_cstring(f):
	chars = []
	c = f.read(1)
	while c != '\0':
		chars.append(c)
		c = f.read(1)
	return ''.join(chars)

def write_cstring(f, s):
	f.write(s + '\0')

def read_byte(f):
	return struct.unpack('B', f.read(1))[0]

def write_byte(f, b):
	assert(0x00 <= b <= 0xFF)
	f.write(struct.pack('B', b))

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
		pad = '\0'
		rest = f.read(6)
		v = struct.unpack('>Q', pad + first + rest)[0] - 0xFC000000000000
	elif (flag & 0xF8) == 0xF8:
		pad = '\0\0'
		rest = f.read(5)
		v = struct.unpack('>Q', pad + first + rest)[0] - 0xF80000000000
	elif (flag & 0xF0) == 0xF0:
		pad = '\0\0\0'
		rest = f.read(4)
		v = struct.unpack('>Q', pad + first + rest)[0] - 0xF000000000
	elif (flag & 0xE0) == 0xE0:
		rest = f.read(3)
		v = struct.unpack('>I', first + rest)[0] - 0xE0000000
	elif (flag & 0xC0) == 0xC0:
		pad = '\0'
		rest = f.read(2)
		v = struct.unpack('>I', pad + first + rest)[0] - 0xC00000
	elif (flag & 0x80) == 0x80:
		rest = f.read(1)
		v = struct.unpack('>H', first + rest)[0] - 0x8000
	else:
		v = struct.unpack('B', first)[0]
	return v

def write_unsigned(f, n):
	if n < 0x80:
		f.write(struct.pack('B', n))
	elif n < 0x4000:
		f.write(struct.pack('>H', n | 0x8000))
	elif n < 0x200000:
		f.write(struct.pack('>I', n | 0xC00000)[1:])
	elif n < 0x10000000:
		f.write(struct.pack('>I', n | 0xE0000000))
	elif n < 0x8000000000:
		f.write(struct.pack('>Q', n | 0xF000000000)[3:])
	elif n < 0x400000000000:
		f.write(struct.pack('>Q', n | 0xF80000000000)[2:])
	elif n < 0x20000000000000:
		f.write(struct.pack('>Q', n | 0xFC000000000000)[1:])
	elif n < 0x1000000000000000:
		f.write(struct.pack('>Q', n | 0xFE00000000000000))
	else:
		write_byte(f, 0xFF)
		f.write(struct.pack('>Q', n))

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
		pad = '\0'
		rest = f.read(2)
		v = -(struct.unpack('>I', pad + first + rest)[0] - 0xF80000)
	elif (flag & 0xF0) == 0xF0:
		pad = '\0'
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

def write_signed(f, n):
	if n < 0:
		nn = -n
		if nn < 0x40:
			f.write(struct.pack('B', nn | 0x80))
		elif nn < 0x1000:
			f.write(struct.pack('>H', nn | 0xE000))
		elif nn < 0x040000:
			f.write(struct.pack('>I', nn | 0xF80000)[1:])
		elif nn < 0x01000000:
			f.write(struct.pack('>I', nn | 0xFE000000))
		else:
			write_byte(f, 0xFF)
			f.write(struct.pack('>I', nn | 0x80000000))
	else:
		if n < 0x80:
			f.write(struct.pack('B', n))
		elif n < 0x2000:
			f.write(struct.pack('>H', n | 0xC000))
		elif n < 0x080000:
			f.write(struct.pack('>I', n | 0xF00000)[1:])
		elif n < 0x02000000:
			f.write(struct.pack('>I', n | 0xFC000000))
		else:
			write_byte(f, 0xFF)
			f.write(struct.pack('>I', n))

def input_file_lines(filename):
	with open(filename, 'r') as input_file:
		for line in input_file.xreadlines():
			line = line.split('#', 2)[0].strip()
			if line:
				yield line

if len(sys.argv) < 2:
	print(__doc__[1:-1])
	sys.exit(1)

# Prepend "converted_binary_" or "converted_text_" to the input filename
# and replace extension with ".vbm" or ".txt" for the output file
filepath = sys.argv[1]
dirname, filename = os.path.split(filepath)
filename_noext, ext = os.path.splitext(filename)
convert_to_binary = ext.lower() == '.txt'
if convert_to_binary:
	new_filepath = os.path.join(dirname, 'converted_binary_' + filename_noext + '.vbm')
else:
	new_filepath = os.path.join(dirname, 'converted_text_' + filename_noext + '.txt')

if convert_to_binary:
	# Convert text file to binary file
	with open(new_filepath, 'wb') as new_f:
		lines = input_file_lines(filepath)
		
		# Write file signature
		new_f.write(BINARY_FILE_SIGNATURE)
		
		# Write file format version
		write_byte(new_f, 1)
		
		# Write comment
		write_cstring(new_f, 'Converted from ' + os.path.basename(filepath))
		
		# Write soma types
		num_types = int(next(lines))
		write_byte(new_f, num_types)
		for _ in xrange(num_types):
			t_id, t_letter = next(lines).split()
			write_byte(new_f, ord(t_letter))
		
		# Write somas
		num_somas = int(next(lines))
		write_unsigned(new_f, num_somas)
		for _ in xrange(num_somas):
			s_type, s_id, s_x, s_y, s_z, s_a, s_d = next(lines).split()
			num_axonal = int(s_a)
			num_dendritic = int(s_d)
			write_byte(new_f, int(s_type))
			write_unsigned(new_f, int(s_id))
			write_signed(new_f, int(s_x))
			write_signed(new_f, int(s_y))
			write_signed(new_f, int(s_z))
			write_unsigned(new_f, num_axonal)
			write_unsigned(new_f, num_dendritic)
			for _a in xrange(num_axonal):
				a_x1, a_x2, a_y1, a_y2, a_z1, a_z2 = next(lines).split()
				write_signed(new_f, int(a_x1))
				write_signed(new_f, int(a_x2))
				write_signed(new_f, int(a_y1))
				write_signed(new_f, int(a_y2))
				write_signed(new_f, int(a_z1))
				write_signed(new_f, int(a_z2))
			for _d in xrange(num_dendritic):
				d_x1, d_x2, d_y1, d_y2, d_z1, d_z2 = next(lines).split()
				write_signed(new_f, int(d_x1))
				write_signed(new_f, int(d_x2))
				write_signed(new_f, int(d_y1))
				write_signed(new_f, int(d_y2))
				write_signed(new_f, int(d_z1))
				write_signed(new_f, int(d_z2))
		
		# Write synapses
		num_synapses = int(next(lines))
		write_unsigned(new_f, num_synapses)
		for _ in xrange(num_synapses):
			line = next(lines)
			if 'v' in line:
				y_id, y_v, y_a, y_d, y_vx, y_vy, y_vz, y_x, y_y, y_z = line.split()
				write_unsigned(new_f, int(y_id))
				write_byte(new_f, 1)
				write_unsigned(new_f, int(y_a))
				write_unsigned(new_f, int(y_d))
				write_signed(new_f, int(y_vx))
				write_signed(new_f, int(y_vy))
				write_signed(new_f, int(y_vz))
				write_signed(new_f, int(y_x))
				write_signed(new_f, int(y_y))
				write_signed(new_f, int(y_z))
			else:
				y_id, y_a, y_d, y_x, y_y, y_z = line.split()
				write_unsigned(new_f, int(y_id))
				write_byte(new_f, 0)
				write_unsigned(new_f, int(y_a))
				write_unsigned(new_f, int(y_d))
				write_signed(new_f, int(y_x))
				write_signed(new_f, int(y_y))
				write_signed(new_f, int(y_z))
		
		# Write gap junctions
		try:
			num_gap_junctions = int(next(lines))
		except StopIteration:
			num_gap_junctions = 0
		write_unsigned(new_f, num_gap_junctions)
		for _ in xrange(num_gap_junctions):
			g_s1, g_s2, g_x, g_y, g_z = next(lines).split()
			write_unsigned(new_f, int(g_s1))
			write_unsigned(new_f, int(g_s2))
			write_signed(new_f, int(g_x))
			write_signed(new_f, int(g_y))
			write_signed(new_f, int(g_z))
		
else:
	# Convert binary file to text file
	with open(filepath, 'rb') as f, open(new_filepath, 'w') as new_f:
		
		# Read file signature
		f_sig = f.read(5)
		assert(f_sig == BINARY_FILE_SIGNATURE)
		
		# Read file format version
		f_ver = read_byte(f)
		assert(f_ver > 0 and f_ver <= MAX_BINARY_FILE_FORMAT_VERSION)
		
		# Read comment
		comment = "Converted from " + os.path.basename(filepath) + " (version " + str(f_ver) + ")\n"
		comment += read_cstring(f).replace('\r', '\n')
		new_f.write(''.join(['# ' + line + '\n' for line in comment.split('\n') if line]))
		
		# Read soma types
		num_types = read_byte(f)
		new_f.write('# Types Start (first line is the total count to follow)\n')
		new_f.write(str(num_types) + '\n')
		new_f.write('# <id> <letter>\n')
		for t_id in xrange(num_types):
			t_letter = f.read(1)
			new_f.write(str(t_id) + ' ' + t_letter + '\n')
		
		# Read somas
		num_somas = read_unsigned(f)
		if f_ver >= 2:
			num_fields = read_unsigned(f)
		new_f.write('# Somas Start (first line is the total count to follow)\n')
		new_f.write(str(num_somas) + '\n')
		new_f.write('# <type id> <id> <x> <y> <z> <number of axonal fields> <number of dendritic fields>\n')
		new_f.write('#     <x1> <x2> <y1> <y2> <z1> <z2>\n')
		for _ in xrange(num_somas):
			s_type = read_byte(f)
			s_id = read_unsigned(f)
			s_x = read_signed(f)
			s_y = read_signed(f)
			s_z = read_signed(f)
			s_a = read_unsigned(f)
			s_d = read_unsigned(f)
			new_f.write(str(s_type) + ' ' + str(s_id) + ' ' + str(s_x) + ' ' + str(s_y) + ' ' + str(s_z) + ' ' + str(s_a) + ' ' + str(s_d) + '\n')
			for _a in xrange(s_a):
				a_x1 = read_signed(f)
				a_x2 = read_signed(f)
				a_y1 = read_signed(f)
				a_y2 = read_signed(f)
				a_z1 = read_signed(f)
				a_z2 = read_signed(f)
				new_f.write('\t' + str(a_x1) + ' ' + str(a_x2) + ' ' + str(a_y1) + ' ' + str(a_y2) + ' ' + str(a_z1) + ' ' + str(a_z2) + '\n')
			for _d in xrange(s_d):
				d_x1 = read_signed(f)
				d_x2 = read_signed(f)
				d_y1 = read_signed(f)
				d_y2 = read_signed(f)
				d_z1 = read_signed(f)
				d_z2 = read_signed(f)
				new_f.write('\t' + str(d_x1) + ' ' + str(d_x2) + ' ' + str(d_y1) + ' ' + str(d_y2) + ' ' + str(d_z1) + ' ' + str(d_z2) + '\n')
		
		# Read synapses
		num_synapses = read_unsigned(f)
		new_f.write('# Synapses Start (first line is the total count to follow)\n')
		new_f.write(str(num_synapses) + '\n')
		new_f.write('# <id> <axonal id> <dendritic id> <x> <y> <z>\n')
		new_f.write('# or\n')
		new_f.write("# <id> 'v' <axonal id> <dendritic id> <vx> <vy> <vz> <x> <y> <z>\n")
		for _ in xrange(num_synapses):
			y_id = read_unsigned(f)
			has_via = read_byte(f)
			if has_via:
				y_a = read_unsigned(f)
				y_d = read_unsigned(f)
				y_vx = read_signed(f)
				y_vy = read_signed(f)
				y_vz = read_signed(f)
				y_x = read_signed(f)
				y_y = read_signed(f)
				y_z = read_signed(f)
				new_f.write(str(y_id) + ' ' + 'v ' + str(y_a) + ' ' + str(y_d) + ' ' + str(y_vx) + ' ' + str(y_vy) + ' ' + str(y_vz) + ' ' + str(y_x) + ' ' + str(y_y) + ' ' + str(y_z) + '\n')
			else:
				y_a = read_unsigned(f)
				y_d = read_unsigned(f)
				y_x = read_signed(f)
				y_y = read_signed(f)
				y_z = read_signed(f)
				new_f.write(str(y_id) + ' ' + str(y_a) + ' ' + str(y_d) + ' ' + str(y_x) + ' ' + str(y_y) + ' ' + str(y_z) + '\n')
		
		# Read gap junctions
		num_gap_junctions = read_unsigned(f)
		new_f.write('# Gap Junctions Start (first line is the total count to follow)\n')
		new_f.write(str(num_gap_junctions) + '\n')
		new_f.write('# <soma1 id> <soma2 id> <x> <y> <z>\n')
		for _ in xrange(num_gap_junctions):
			g_s1 = read_unsigned(f)
			g_s2 = read_unsigned(f)
			g_x = read_signed(f)
			g_y = read_signed(f)
			g_z = read_signed(f)
			new_f.write(str(g_s1) + ' ' + str(g_s2) + ' ' + str(g_x) + ' ' + str(g_y) + ' ' + str(g_z) + '\n')

os.utime(__file__, None) # touch convert-model.py

#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: strip-first-column.py FILENAME

Strips the first space-delimited column from the lines in a console file.

Example: strip-first-column.py 0_of_1_weights_20140317144808.txt
This outputs stripped_0_of_1_weights_20140317144808.txt.
"""

from __future__ import print_function, with_statement

import sys
import os
import os.path

if len(sys.argv) < 2:
	print(__doc__[1:-1])
	sys.exit(1)

# Prepend "stripped_" to the input filename for the output file
filepath = sys.argv[1]
dirname, filename = os.path.split(filepath)
new_filepath = os.path.join(dirname, 'stripped_' + filename)

# Remove first space-delimited column characters from each line
# Example:
#     0 1 17 3 89 20 23
# becomes:
#     1 17 3 89 20 23
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	for line in f:
		new_f.write(line[line.find(' ')+1:])

os.utime(__file__, None) # touch strip-first-column.py

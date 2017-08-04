#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: strip-timestamp.py FILENAME

Strips the timestamps from the lines in a console file.

Example: strip-timestamp.py 0_of_1_console_20140601163509.txt
This outputs stripped_0_of_1_console_20140601163509.txt.
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

# Remove first 22 characters from each line
# Example:
#     2014-06-01 16:35:09 | INFO | Log level set to: 2
# becomes:
#     INFO | Log level set to: 2
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	for line in f:
		new_f.write(line[22:])

os.utime(__file__, None) # touch strip-timestamp.py

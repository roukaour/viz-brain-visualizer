#!/usr/bin/env python
# Works with Python 2.7, not 3.3

"""
Usage: summarize-firings.py FILENAME START_ID END_ID

Outputs statistics about the spike counts in a firings file for somas with
IDs within a given range.

Example:
summarize-firings.py 0_of_1_firings_20140601163509.txt 2012474 2022473
This outputs a histogram of firing spike counts for the somas with IDs
#2012474 to #2022473.
"""

from __future__ import print_function, with_statement

import sys
import os.path

if len(sys.argv) < 4:
	print(__doc__[1:-1])
	sys.exit(1)

filepath = sys.argv[1]
start_id = int(sys.argv[2])
end_id = int(sys.argv[3])

soma_ids = set(range(start_id, end_id+1))
spike_counts = {id: 0 for id in soma_ids}

with open(filepath, 'r') as f:
	# Skip first 9 lines
	# Example:
	#     # CRC for input file = 2947219980, excludes first line and line endings.
	#     v 2
	#     # Next line contains the time step in microseconds (100 - 10000):
	#     500
	#     # Next line contains the total soma count:
	#     2423776
	#     # Next line contains the number of cycles:
	#     2000
	#     # Cycle number <space> Number of SomaIds <space> SomaId1 <space> SomaId1_State <space> SomaId2 <space> SomaId2_State <space> ... SomaIdN <space> SomaIdN_State
	for _ in range(9):
		f.readline()
	
	# Count soma IDs
	for line in f:
		firing_ids = set([int(id) for id in line.split()[2::2]])
		for firing_id in firing_ids:
			if firing_id in soma_ids:
				spike_counts[firing_id] += 1

# Print summary statistics of spike counts
count_values = spike_counts.values()
min_count = min(count_values)
max_count = max(count_values)
avg_count = sum(count_values) / float(len(spike_counts)) if spike_counts else 0
print('%d somas with IDs from %d to %d' % (len(soma_ids), start_id, end_id))
print('Firing counts from %d to %d, average %.2f' % (min_count, max_count, avg_count))
print()

# Print histogram of spike counts
print('#firings\t#somas')
for count in range(min_count, max_count+1):
	print('%d\t%d' % (count, count_values.count(count)))
print()

# Print list of spike counts
print('ID\t#firings')
for soma_id in sorted(soma_ids):
	print('%d\t%d' % (soma_id, spike_counts[soma_id]))

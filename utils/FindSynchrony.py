#!/usr/bin/env python
# Works with Python 2.7, not 3.3
#needs scipy for some calculations

"""
Usage: FindSynchrony.py FILENAME

Finds the synchrony given a voltage file
"""

#from __future__ import print_function, with_statement

import sys
import os
import os.path
#from statistics import mean
#import scipy
#from scipy import stats
import scipy as sp

if len(sys.argv) < 2:
	print(__doc__[1:-1])
	sys.exit(1)

# Create an output file, currently used for debugging
filepath = sys.argv[1]
#filepath = "0_of_1_voltages_20140702165731.txt"
#filepath = "0_of_1_voltages_20140707133136.txt"
print filepath
dirname, filename = os.path.split(filepath)
filename_noext = os.path.splitext(filename)[0]
new_filepath = os.path.join(dirname, 'test' + filename_noext + '.txt')

# Read all rows, every cycle contains all voltages for selected cells
with open(filepath, 'r') as f, open(new_filepath, 'w') as new_f:
	# Skip first 3 lines
	# Example:
	#     # Next line contains the total soma count (includes inactive):
        #1188894
      
	for _ in range(3):
		f.readline()
	#number of neurons being reported 
	
	neurons = int(f.readline())
#	new_f.write(str(neurons))
        # Skip 3 more lines 
	for _ in range(3):
		f.readline()
	# number of cycles
	cyclesStr = f.readline()
	cycles = int(cyclesStr)
	for _ in range(3):
		f.readline()
	print 'Number of neurons=', neurons

	CycleVoltages = [[]]
	TotalV = []
	odd=1
	avgVar1 =0.0
	avgVar2 = 0.0
	avgVariance = 0.0
	cellVar1 = [0,0,0,0,0]
	cellVar2 = [0,0,0,0,0]
	cellavgV = 0.0
	synch = 0.0
	#cycles = 100
	
	for line in f:	
		tokens = line.split()
		cycle, VoltagesLine = tokens[0], tokens[1:]
		CycleVoltages.append([])
		TotalV.append(0)
		
		for odd in range(0, 2*neurons, 2):
		    CycleVoltages[int(cycle)].append(float(VoltagesLine[odd]))
		    
		new_f.write(str(CycleVoltages[int(cycle)]))
		avgV = sp.mean((CycleVoltages[int(cycle)]))
		
		new_f.write("\tmean=")
		new_f.write(str(avgV))
		new_f.write("\n")
		avgVar1 = (avgVar1 + avgV*avgV)
		avgVar2 = avgVar2 + avgV
		
		for k in range(0, neurons):
		    cellVar1[k] = cellVar1[k] + (CycleVoltages[int(cycle)][k] * CycleVoltages[int(cycle)][k] )
		    cellVar2[k] = cellVar2[k] + CycleVoltages[int(cycle)][k]
	
	#avgVariance = avgVariance/int(cycles)	
	avgVar2 = avgVar2/cycles
	# Top of Synchrony formula
	avgVariance = avgVar1/cycles - (avgVar2*avgVar2)
	new_f.write("\n avgVariance=")
	new_f.write(str(avgVariance))
	for k in range(0, neurons):
	        cellVar2[k] = cellVar2[k]/int(cycles)
		cellavgV = cellavgV + cellVar1[k]/int(cycles) - cellVar2[k]*cellVar2[k]
	new_f.write("\n cellVar1[0]=")
	new_f.write(str(cellVar1[0]))
	new_f.write("\n cellVar2[0]=")
	new_f.write(str(cellVar2[0]))
	new_f.write("\n cellavgV=")
	new_f.write(str(cellavgV))
	# bottom of Synchrony formula    
	cellavgV = cellavgV/float(neurons)
	if cellavgV != 0:
	   synch = avgVariance/cellavgV
	else:
	   synch = 1
		
	new_f.write('\n')
	new_f.write(str(sp.mean((CycleVoltages[int(cycle)]))))
	new_f.write('\n')
	new_f.write("avgVariance=")
	new_f.write(str(avgVariance))
	new_f.write('\n')
	new_f.write("cellavgV=")
	new_f.write(str(cellavgV))
	new_f.write('\n')
	new_f.write("Synchrony=")
	new_f.write(str(synch))
	print 'Synchrony= ', str(synch)

os.utime(__file__, None) # touch filter-firings.py

#!/bin/python

import pickle
import sys

class Datapoint:
    def __init__(self):
        self.real = '.'
        self.predicted = '.'

datapoints=[]

# reads in test directions
def readFile(fname):

    fd = open(fname, 'r')
    lines = fd.readlines()
    for line in lines:
        if len(line)>0:
       	    tmp = line.split()
            if len(tmp)>0:
                dp = Datapoint() 
                dp.real = float(tmp[1])
                dp.predicted = float(tmp[2])
                datapoints.append(dp)
    fd.close()


database_file = readFile(sys.argv[1])
#database_file = readFile('data/linear-lambda-0.1.dat')

n=0
sum_MAE=0
sum_S_MAE=0
sum_S_MAE_absolute=0
for dp in datapoints:
    n=n+1
    # MAE
    sum_MAE=sum_MAE+abs(dp.predicted-dp.real)
    # S-MAE (10%)
    if  abs(dp.predicted-dp.real)>0.1*dp.real:
	sum_S_MAE=sum_S_MAE+abs(dp.predicted-dp.real)
    # sum_S_MAE_absolute (300 seconds)
    if  abs(dp.predicted-dp.real)>300:
	sum_S_MAE_absolute=sum_S_MAE_absolute+abs(dp.predicted-dp.real)

print(str(sum_MAE/n) + "\t" + str(sum_S_MAE/n) + "\t" + str(sum_S_MAE_absolute/n))
#print(sum_S_MAE/n)
#print(sum_S_MAE_absolute/n)
	

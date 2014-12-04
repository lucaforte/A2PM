#!/bin/python

import pickle
import sys
import os
import csv

class Datapoint:
    def __init__(self):
        self.real = '.'
        self.predicted = '.'

# reads in test directions
def readFile(fname):
    i=0;
    fd = open(fname, 'r')
    lines = fd.readlines()
    for line in lines:
        if len(line)>0 and i>0 and i<3748:
       	    tmp = line.split(',')
            if len(tmp)>0:
                dp = Datapoint() 
                dp.predicted = float(tmp[0])
                dp.real = float(tmp[1])
                datapoints.append(dp)
	i=i+1
    fd.close()

datapoints=[]
file = readFile(sys.argv[1])

n=0
sum_average_real_values_RAPE=0
for dp in datapoints:
    n=n+1
    sum_average_real_values_RAPE=sum_average_real_values_RAPE+dp.real
average_real_values_RAPE=sum_average_real_values_RAPE/n
max_error=0
sum_numerator_RAPE=0
sum_denominator_RAPE=0
sum_MAE=0
sum_S_MAE=0
sum_S_MAE_absolute=0

for dp in datapoints:
	    
    # Maximum Absolute Prediction Error
    if max_error<abs(dp.predicted-dp.real):
	max_error=abs(dp.predicted-dp.real)	
    # Relative Absolute Prediction Error
    sum_numerator_RAPE=sum_numerator_RAPE+abs(dp.predicted-dp.real)
    sum_denominator_RAPE=sum_denominator_RAPE+abs(average_real_values_RAPE-dp.real)
    # MAE
    sum_MAE=sum_MAE+abs(dp.predicted-dp.real)
    # S-MAE (10%)
    if  abs(dp.predicted-dp.real)>0.1*dp.real:
	sum_S_MAE=sum_S_MAE+abs(dp.predicted-dp.real)
    # sum_S_MAE_absolute (300 seconds)
    if  abs(dp.predicted-dp.real)>300:
	sum_S_MAE_absolute=sum_S_MAE_absolute+abs(dp.predicted-dp.real)

print(str(max_error) + "\t" +  str(sum_numerator_RAPE/sum_denominator_RAPE*100) + " %\t" +  str(sum_MAE/n) + "\t" + str(sum_S_MAE/n) + "\t" + str(sum_S_MAE_absolute/n))


	

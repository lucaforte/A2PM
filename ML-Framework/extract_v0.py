#!/bin/python

import pickle
import sys


class Datapoint:
    "Shows datapoint for one moment in time. Records Mem,Swap,CPU"
    def __init__(self):
##        self.mem_total = '.'
        self.mem_used = '.'
        self.mem_free = '.'
        self.mem_shared = '.'
        self.mem_buffers = '.'
        self.mem_cached = '.'
##        self.swap_total = '.'
        self.swap_used = '.'
        self.swap_free = '.'
        self.cpu_user = '.'
        self.cpu_nice = '.'
        self.cpu_system = '.'
        self.cpu_iowait = '.'
        self.cpu_steal = '.'
        self.cpu_idle = '.'



# reads in test directions
def readDirs(fname):
    run =-1
    datapoint =0
    history=[[]]
    dp = Datapoint()
    fd = open(fname, 'r')
    lines = fd.readlines()
    for line in lines:
        if len(line)>0:
            if line=='------------NEW ACTIVE MACHINE:----------\n':
                #save the current datapoint
                history.append([])
                print run,datapoint
                history[run].append(dp)
                run+=1
                datapoint =0
                dp = Datapoint()
            else:
                tmp = line.split()
                if len(tmp)<1:
                    do_nothing=0  
                elif tmp[0] == 'Datapoint:':
                    #save datapoint
                    if datapoint>0:
                        history[run].append(dp)
                    dp = Datapoint()
                    datapoint +=1;
                elif tmp[0] == 'Memory:':
##                    dp.mem_total = tmp[1]
                    dp.mem_used = float(tmp[2])
                    dp.mem_free = float(tmp[3])
                    dp.mem_shared = float(tmp[4])
                    dp.mem_buffers = float(tmp[5])
                    dp.mem_cached = float(tmp[6])
                elif tmp[0] == 'Swap:':
##                    dp.swap_total = tmp[1]
                    dp.swap_used = float(tmp[2])
                    dp.swap_free = float(tmp[3])
                elif tmp[0] == 'CPU:':
                    dp.cpu_user = float(tmp[1])
                    dp.cpu_nice = float(tmp[2])
                    dp.cpu_system = float(tmp[3])
                    dp.cpu_iowait = float(tmp[4])
                    dp.cpu_steal = float(tmp[5])
                    dp.cpu_idle = float(tmp[6])         
    fd.close()
    return history


if len(sys.argv) < 2:
	print "You must specify the point database file"
	exit()

database_file = readDirs(sys.argv[1])

history=[]
history.extend(database_file)

print 'examples=',len(history)

def long_enough(x):
    if len(x)>500:
        return True
    else:
        return False
    

history = filter(long_enough,history)



##removing data point after points where system is overloaded

def checkZeroSlopes(dp):
    ans = []
    ans.append(dp.mem_used)
    ans.append(dp.mem_free)
    ans.append(dp.mem_shared)
    ans.append(dp.mem_buffers)
    ans.append(dp.mem_cached)
    ans.append(dp.swap_used)
    ans.append(dp.swap_free)
    ans.append(dp.cpu_user)
    ans.append(dp.cpu_nice)
    ans.append(dp.cpu_system)
    ans.append(dp.cpu_iowait)
    ans.append(dp.cpu_steal)
    ans.append(dp.cpu_idle)
    return ans

aggregate_constant = 30
A = aggregate_constant

new_dataset = []




for run in history:
    for k in range(len(run)/A):
        last = len(run)-k*A-1
        set_ind =[last-i for i in range(A)]
        nwdp =[run[i] for i in set_ind]
        nwdp = (k,'times',A,'seconds',nwdp)
        new_dataset.append(nwdp)
		
ndp = new_dataset[50]

def dpToList(dp):
    ans = []
    ans.append(dp.mem_used)
    ans.append(dp.mem_free)
    ans.append(dp.mem_shared)
    ans.append(dp.mem_buffers)
    ans.append(dp.mem_cached)
    ans.append(dp.swap_used)
    ans.append(dp.swap_free)
    ans.append(dp.cpu_user)
    ans.append(dp.cpu_nice)
    ans.append(dp.cpu_system)
    ans.append(dp.cpu_iowait)
    ans.append(dp.cpu_steal)
    ans.append(dp.cpu_idle)
    return ans

def listToDp(tmp):
    dp = Datapoint()
    dp.mem_used = tmp[0]
    dp.mem_free = tmp[1]
    dp.mem_shared = tmp[2]
    dp.mem_buffers = tmp[3]
    dp.mem_cached = tmp[4]
    dp.swap_used = tmp[5]
    dp.swap_free = tmp[6]
    dp.cpu_user = tmp[7]
    dp.cpu_nice = tmp[8]
    dp.cpu_system = tmp[9]
    dp.cpu_iowait = tmp[10]
    dp.cpu_steal = tmp[11]
    dp.cpu_idle = tmp[12]
    return dp

    
def slope(ndp):
    n = len(ndp[4])
    st = ndp[4][0]
    end = ndp[4][n-1]
    st_l =  dpToList(st)
    end_l = dpToList(end)
    ans = []
    for i in range(len(st_l)):
        next = (-end_l[i]+st_l[i]+0.00)/n
        ans.append(next)
    slp = listToDp(ans)
    return slp
    
    
def extra_features(ndp):
    return (slope(ndp),ndp)

	
def extToMatlab(extra):
    ttl = extra[1][0]*extra[1][2]
    ag = aggregate_constant/2
    slope = extra[0]
    median = extra[1][4][ag]
    last= extra[1][4][aggregate_constant-1]
    return (ttl,slope,median,last)



def printSlope(i):
    ndd = new_dataset[i]
    sl = slope(ndd)
    ze = dpToList(sl)
    return ze

ex = extra_features(new_dataset[15])
exM = extToMatlab(ex)

def printEXM(exm):
    output=""
    output+=str(exm[0])+','
    output+=str(dpToList(exm[1]))[1:-1]+','
    output+=str(dpToList(exm[2]))[1:-1]+','
    output+=str(dpToList(exm[3]))[1:-1]
    return output

print printEXM(exM)
print len(new_dataset)

def legend(str):
    ans=""
    ans+='mem_used'+str+','
    ans+='mem_free'+str+','
    ans+='mem_shared'+str+','
    ans+='mem_buffers'+str+','
    ans+='mem_cached'+str+','
    ans+='swap_used'+str+','
    ans+='swap_free'+str+','
    ans+='cpu_user'+str+','
    ans+='cpu_nice'+str+','
    ans+='cpu_system'+str+','
    ans+='cpu_iowait'+str+','
    ans+='cpu_steal'+str+','
    ans+='cpu_idle'+str
    return ans

    
fout = open('aggregated_v0.csv','w')
fout.write(',')
fout.write(legend("_slope")+',')
fout.write(legend("_15")+',')
fout.write(legend("_30")+'\n')           
for i in range(len(new_dataset)):
    ex=extra_features(new_dataset[i])
    exM=extToMatlab(ex)
    nextline=printEXM(exM)
    print(i)
    fout.write(nextline+"\n")
fout.close()






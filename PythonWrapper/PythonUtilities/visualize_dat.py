#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Nov 13 10:00:26 2018
Helper script to visualize INCABuilder .dat format

@author: jose-luis
"""

import pandas as pd
import re
import numpy as np
import datetime as dt

#Reading in file
f = open('../SimplyC/langtjerninputs.dat','r')
fileStr = f.read()
f.close() 
   
#Function to read a group of text, either one with all timesteps or delimited with "end_timeseries"
block_re = '(?<="{}" :\\n)(.+\\n)+'
def readGroup(name):
    group_re = re.compile(block_re.format(name))
    data = re.search(group_re,fileStr)
    return np.array(list(map(float,data.group().strip().split('\n'))))

subblock_re = '(?<="{}" :\\n)(.+\\n)+(?=end_timeseries)'
def readSubGroup(name):
    subgroup_re = re.compile(subblock_re.format(name))
    data = re.search(subgroup_re,fileStr)
    data = data.group().strip().split('\n')
    data = [re.split('[\\t ]+',i) for i in data]
    data = [[dt.datetime.strptime(i[0],'"%Y-%m-%d"'),float(i[1])] for i in data]
    df = pd.DataFrame({'date': [i[0] for i in data],
                       name : [i[1] for i in data]})
    df.set_index('date', inplace=True)
    return df.drop_duplicates()
    

#Getting start date    
start_date_re = re.compile("(?<=^start_date : ).*")
start_date = re.search(start_date_re,fileStr)
start_date = dt.datetime.strptime(start_date.group(),'"%Y-%m-%d"')

#Getting number of timesteps
timesteps_re = re.compile("(?<=timesteps : ).*")
timesteps = re.search(timesteps_re,fileStr)
timesteps = int(timesteps.group())

#Creating array with all timestamps
date_idx = np.array(pd.date_range(start_date,periods=timesteps))

#Getting the different variables:
temperature = readGroup("Air temperature")
precipitation = readGroup("Precipitation")
discharge = readGroup("Discharge")

#Reading delimited timeseries
TOC = readSubGroup('TOC')

#Putting all data in a dataframe
df = pd.DataFrame({'date' : date_idx, 'temperature' :  temperature, 'precipitation' : precipitation, 'discharge' : discharge })
df.set_index('date',inplace=True)

data = pd.concat([df,TOC],axis=1)

ax = data.plot(subplots=True)
ax[3].get_lines()[0].set_marker('.')
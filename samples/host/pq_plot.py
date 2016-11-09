#!/usr/bin/env python3


import sys
import csv
import pandas
import matplotlib.pyplot as plt
import matplotlib



filename = sys.argv[1]

colnames = ['value']
data = pandas.read_csv(filename, names=colnames)
values = data['value']


matplotlib.style.use('ggplot')


plt.plot(values)

plt.show()


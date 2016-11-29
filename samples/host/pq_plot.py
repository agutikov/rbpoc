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

# adc shift
values /= 16

#adc gain
values /= 2/3

# amp gain
values /= 1100





matplotlib.style.use('ggplot')


plt.plot(values)

plt.show()


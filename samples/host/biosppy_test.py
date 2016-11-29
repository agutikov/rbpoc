#!/usr/bin/env python2.7

import sys
import numpy as np
import pandas
from biosppy.signals import ecg
import matplotlib.pyplot as plt
import matplotlib
import biosppy.signals.tools as st
from biosppy import plotting, utils


sampling_rate=920

filename = sys.argv[1]

colnames = ['value']
data = pandas.read_csv(filename, names=colnames)
raw = data['value']

ecg.ecg(signal=raw, sampling_rate=sampling_rate, show=True)


order = int(0.07 * sampling_rate)
filtered1, _, _ = st.filter_signal(signal=raw,
                                  ftype='FIR',
                                  band='bandpass',
                                  order=order,
                                  frequency=[2, 5],
                                  sampling_rate=sampling_rate)

order = int(0.1 * sampling_rate)
filtered2, _, _ = st.filter_signal(signal=raw,
                                  ftype='FIR',
                                  band='lowpass',
                                  order=order,
                                  frequency=40,
                                  sampling_rate=sampling_rate)

rpeaks, = ecg.hamilton_segmenter(signal=filtered2, sampling_rate=sampling_rate)
# rpeaks, = ecg.christov_segmenter(signal=filtered, sampling_rate=sampling_rate)
# rpeaks, = ecg.ssf_segmenter(signal=filtered, sampling_rate=sampling_rate,
#                            threshold=200, before=1, after=1)




fig = plt.figure(1)

ymin = np.min(raw)
ymax = np.max(raw)

ax1 = plt.subplot(311)
ax1.plot(raw)
ax1.vlines(rpeaks, ymin, ymax, color='m')


ymin = np.min(filtered1)
ymax = np.max(filtered1)

ax2 = plt.subplot(312, sharex=ax1)
ax2.plot(filtered1)
ax2.vlines(rpeaks, ymin, ymax, color='m')



ymin = np.min(filtered2)
ymax = np.max(filtered2)

ax2 = plt.subplot(313, sharex=ax1)
ax2.plot(filtered2)
ax2.vlines(rpeaks, ymin, ymax, color='m')


mng = plt.get_current_fig_manager()
mng.resize(*mng.window.maxsize())

plt.subplots_adjust(left=0.05, right=0.95, top=0.95, bottom=0.05)
plt.show()

exit()


Fs=920
step = 5
for i in range(0, 60-step, step):
    v = values[Fs*i:(i+step)*Fs]
    out = ecg.ecg(signal=v, sampling_rate=Fs, show=True)
    
    nt = len(out['templates'])
    print nt 
    
    plt.figure(1)
    a = nt*100 + 11
    for i in range(0, len(out['templates'])):
        plt.subplot(a + i)
        plt.plot(out['templates'][i])
        
    plt.show()
    




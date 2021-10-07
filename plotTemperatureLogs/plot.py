import pandas as pd
import matplotlib.pyplot as plt
import clevercsv
import re
import os
from pathlib import Path

# https://stackoverflow.com/questions/3964681/find-all-files-in-a-directory-with-extension-txt-in-python?rq=1
def get_all_filepaths(root_path, ext):
    for root, dirs, files in os.walk(root_path):
        files.sort() # Needed for platform-independent order
        temp = files[2]
        files[2] = files[1]
        files[1] = temp
        for filename in files:
            if filename.lower().endswith(ext):
                yield os.path.join(root, filename)

# https://www.geeksforgeeks.org/python-split-camelcase-string-to-individual-strings/
# Python3 program Split camel case 
# string to individual strings
def camel_case_split(str):
    return re.findall(r'[A-Z](?:[a-z]+|[A-Z]*(?=[A-Z]|$))', str)

iIndexNumberY = -1
for file in get_all_filepaths('./heatTest10-7-2021/', '.csv'):
    iIndexNumberY += 1
    df = clevercsv.read_dataframe(file) # https://pypi.org/project/clevercsv/
    print(df)
    # Modify the df:
    # https://pandas.pydata.org/docs/reference/api/pandas.DataFrame.assign.html
    df = df.assign(steadyClockTime=lambda x: x.steadyClockTime / 1e9) # Convert to seconds from nanoseconds
    df = df.assign(celcius=lambda x: x.celcius / 1000) # Convert to celcius from thousands
    df = df[df['steadyClockTime'] <= 500]

    basename = os.path.basename(file)
    subplot = plt.subplot(1, # x dimension
                          1, # y dimension
                          0 + 1)
    #label = 'Heatsinks ' + ' '.join(camel_case_split(basename)).removesuffix(' Time')
    label = {'timestamps_heatsinksNoFanInYellowPayloadSubscaleHousingAndCloth_unixTime1633576259.csv': 'Heat Sinks',
             'timestamps_heatsinksWithFanInYellowPayloadSubscaleHousingWithPassiveVenting(SidesOpen)_unixTime1633576936.csv': 'Heat Sinks + Ventilation',
             'timestamps_heatsinksWithFanInYellowPayloadSubscaleHousingWithBetterCoolingWithFanWorkingInFromSides_unixTime1633579333.csv': 'Heat Sinks + Ventilation + Active Cooling'}[basename]
    if basename == 'timestamps_heatsinksNoFanInYellowPayloadSubscaleHousingAndCloth_unixTime1633576259.csv':
        # Modify the df to offset by 15 seconds
        df = df.assign(steadyClockTime=lambda x: x.steadyClockTime - 15) # Seconds   # https://thispointer.com/pandas-drop-first-n-rows-of-dataframe/#:~:text=To%20delete%20the%20first%20N,default%20values%20i.e.%20(%3A)%20i.e.

        # Remove the first 15 seconds
        #df = df.iloc[N: , :]
        df = df[df['steadyClockTime'] >= 15] # https://www.geeksforgeeks.org/drop-rows-from-the-dataframe-based-on-certain-condition-applied-on-a-column/
    
    print(label)
    fig = plt.plot(df.loc[:, 'steadyClockTime'], df.loc[:, 'celcius'], label=label)
    if iIndexNumberY == 0:
        plt.title('Temperatures')
    subplot.set_xlabel('seconds')
    subplot.set_ylabel('degrees Celcius')
    plt.legend()
    #plt.xlim(startX, endX)

plt.tight_layout()
#plt.show()
plt.savefig('out.png', bbox_inches='tight',pad_inches = 0, dpi = 1000)

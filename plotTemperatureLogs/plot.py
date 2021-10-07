import pandas as pd
import matplotlib.pyplot as plt
import clevercsv
import re
import os

# https://stackoverflow.com/questions/3964681/find-all-files-in-a-directory-with-extension-txt-in-python?rq=1
def get_all_filepaths(root_path, ext):
    for root, dirs, files in os.walk(root_path):
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
    
    subplot = plt.subplot(1, # x dimension
                          1, # y dimension
                          0 + 1)
    fig = plt.plot(df.loc[:, 'steadyClockTime'], df.loc[:, 'celcius'], label='Heatsinks ' + ' '.join(camel_case_split(os.path.basename(file))).removesuffix(' Time'))
    if iIndexNumberY == 0:
        plt.title('Temperatures')
    subplot.set_xlabel('seconds')
    subplot.set_ylabel('degrees Celcius')
    plt.legend()
    #plt.xlim(startX, endX)

plt.show()

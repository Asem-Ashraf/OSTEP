import matplotlib.pyplot as plt
# import pandas as pd
# from csv import reader
# import numpy as np

with open('graphValues.txt', 'r') as f:
    lines = f.readlines()
# Define the x-values
system_ticks = list(map(int,lines[0][:-1].split(', ')))

# Define the y-values for each function
process_10_ticks = list(map(int,lines[1][:-1].split(', ')))
process_20_ticks = list(map(int,lines[2][:-1].split(', ')))
process_30_ticks = list(map(int,lines[3][:-1].split(', ')))

print(len(system_ticks))
print(len(process_10_ticks))
print(len(process_20_ticks))
print(len(process_30_ticks))

# Create a new figure and axis
fig, ax = plt.subplots()

# Plot each function with a different color and shape
ax.plot(system_ticks, process_10_ticks, 'b-', label='process 1 with 10 tickets')
ax.plot(system_ticks, process_20_ticks, 'r-', label='process 2 with 20 tickets')
ax.plot(system_ticks, process_30_ticks, 'g-', label='process 3 with 30 tickets')

# Add a legend and axis labels
ax.legend()
ax.set_xlabel('system ticks')
ax.set_ylabel('process ticks')
ax.set_title('Lottery scheduling for three processes in XV6')

# Show the plot
plt.show()

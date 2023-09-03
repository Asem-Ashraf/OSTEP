import matplotlib.pyplot as plt
import os

print    ("cat testval.txt | tail -n +20 | wc -l > values.txt")
os.system("cat testval.txt | tail -n +20 | wc -l > values.txt")
# if number_in_fine%4!=0 then take read number_in_fine/4 lines from the file
open_file = open('values.txt', 'r')
# read the first line and convert it into integer
number_in_file = 0 
number_in_file = int(open_file.readline())-3
print(number_in_file)
if number_in_file%8!=0:
    number_in_file = (int(number_in_file/8))*4
print(number_in_file*2)
open_file.close()
# run this shell command "cat testval.txt |grep --perl-regex "10\t\t|20\t\t|30\t\t|otal" | sed -E 's/^.*(Total ticks: |(10|20|30)\t\t)([0-9.]+)/\1\3/'| head -number_in_file | awk '{print $$NF}'| > values.txt"
print    ("cat testval.txt | tail -n +20 |grep --perl-regex \"10\\t\\t|20\\t\\t|30\\t\\t|otal\" | sed -E 's/^.*(Total ticks: |(10|20|30)\\t\\t)([0-9.]+)/\\1\\3/'| head -n"+str(number_in_file)+" | sort -nk1,1 -s| awk '{print $NF}' > values.txt")
os.system("cat testval.txt | tail -n +20 |grep --perl-regex \"10\\t\\t|20\\t\\t|30\\t\\t|otal\" | sed -E 's/^.*(Total ticks: |(10|20|30)\\t\\t)([0-9.]+)/\\1\\3/'| head -n"+str(number_in_file)+" | sort -nk1,1 -s| awk '{print $NF}' > values.txt")

open_file = open('values.txt', 'r')

system_ticks = [0,]
process_10_ticks = [0,]
process_20_ticks = [0,]
process_30_ticks = [0,]

# read the first number_in_file/4 lines into system_ticks as integers
for i in range(int(number_in_file/4)):
    system_ticks.append(int(open_file.readline()))

# read the next number_in_file/4 lines into process_10_ticks
for i in range(int(number_in_file/4)):
    process_10_ticks.append(int(open_file.readline()))

# read the next number_in_file/4 lines into process_20_ticks
for i in range(int(number_in_file/4)):
    process_20_ticks.append(int(open_file.readline()))

# read the next number_in_file/4 lines into process_30_ticks
for i in range(int(number_in_file/4)):
    process_30_ticks.append(int(open_file.readline()))


print(len(system_ticks))
print(len(process_10_ticks))
print(len(process_20_ticks))
print(len(process_30_ticks))
print((system_ticks))
print((process_10_ticks))
print((process_20_ticks))
print((process_30_ticks))

# Create a new figure and axis
fig, ax = plt.subplots(figsize=(16, 9))

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
plt.savefig('../graph/graph.png',dpi=100)
# plt.show()

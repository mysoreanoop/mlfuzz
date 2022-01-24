import re

file = open('allCtrlSigs', mode='r')
lines = file.read().splitlines()
file.close()

#r = re.compile("\w@(\w(\[[0-9]+\])?\.?)+")


c = []
for line in lines:
  c.append(line.split())

def flatten_list(_2d_list):
    flat_list = []
    # Iterate through the outer list
    for element in _2d_list:
        if type(element) is list:
            # If the element is of type list, iterate through the sublist
            for item in element:
                flat_list.append(item)
        else:
            flat_list.append(element)
    return flat_list

d = flatten_list(c)
d = [x.strip("()") for x in d]
r = re.compile("[a-zA-Z_@]+")
d = list(filter(None, d))
final = list(filter(r.match, d))

for item in final:
  print(item)
print(len(final))

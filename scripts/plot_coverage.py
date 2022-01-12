import numpy as np
import matplotlib.pyplot as mp

cov = np.zeros((1300,40), dtype=np.int16)
for i in range(0,37):
  print("Reading file %d" %i)
  file = open("/home/anoop/cov%d" % i, mode = "r")
  lines = file.readlines()
  file.close()
  k=0
  while(True):
    try:
      index = lines.index("DONE\n")
    except:
      print("File parsed!")
      break
    else:
      del lines[index]
      cov[k][i] = index
    k=k+1


mp.plot(cov)
mp.xlabel("Number of test cases")
mp.ylabel("Coverage (num toggles)")
mp.show()

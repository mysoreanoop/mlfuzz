import numpy as np
import matplotlib.pyplot as mp

cov = np.zeros((3000,74), dtype=np.int16)
for i in range(0,73):
  print("Reading file %d" %i)
  file = open("/home/anoop/mlfuzz/.coverage/newExpr/cov%d" % (i), mode = "r")
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


mp.plot([cov[x] for x in range(0,300)])
mp.xlabel("Number of test cases")
mp.ylabel("Coverage (num toggles)")
mp.show()

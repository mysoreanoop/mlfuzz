import numpy as np
import matplotlib.pyplot as mp

nListSize = 4584
nCovSize = 500

import os
pwd = os.system('pwd')
pathToCovFiles = f"{pwd}/../.cov/cov"

lists=\
    list(range(0,nListSize))
cov = np.zeros((nCovSize, len(lists)), dtype=np.int16)
m=0
for i in lists:
  #print("Reading file %d" %i)
  m+=1
  try:
    with open(pathToCovFiles + "%d" % (i), mode = "r") as file:
      lines = file.readlines()
      file.close()
      k=0
      while(True):
        try:
          index = lines.index("Complete\n")
        except:
          #print("File parsed!")
          break
        else:
          del lines[index]
          cov[k][m] = index
        k=k+1
  except:
    pass


for i in range(0,nCovSize):
  maxi = max(cov[:][i]) + 1
  for j in range(cov.shape[0]):
    #cov[j][i] = cov[j][i] - cov[0][i]
    print(cov[j][i], end=" ")
  print(" ")

mp.matshow(cov[:][:])
mp.colorbar()
mp.clim(0, 100)
#mp.plot([cov[x] for x in range(0,nListSize)])
#mp.xlabel("Number of test cases")
#mp.ylabel("Coverage (num toggles)")
mp.show()

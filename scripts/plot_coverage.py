import numpy as np
import matplotlib.pyplot as mp


lists=\
    list(range(0,115))\
    +list(range(400,416))\
    +list(range(1000,1073))\
    +list(range(3000,3090))
cov = np.zeros((500,len(lists)), dtype=np.int16)
m=0
for i in lists:
  #print("Reading file %d" %i)
  m+=1
  try:
    with open("/home/anoop/cov%d" % (i), mode = "r") as file:
      lines = file.readlines()
      file.close()
      k=0
      while(True):
        try:
          index = lines.index("DONE\n")
        except:
          #print("File parsed!")
          break
        else:
          del lines[index]
          cov[k][m] = index
        k=k+1
  except:
    pass


for i in range(0,20):
  maxi = max(cov[:][i]) + 1
  for j in range(0,5):#cov.shape[0]):
    #cov[j][i] = cov[j][i] - cov[0][i]
    print(cov[j][i], end=" ")
  print(" ")

mp.matshow(cov[:][:])
mp.colorbar()
mp.clim(0, 100)
#mp.plot([cov[x] for x in range(0,500)])
#mp.xlabel("Number of test cases")
#mp.ylabel("Coverage (num toggles)")
mp.show()

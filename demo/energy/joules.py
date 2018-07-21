import time
import sys
sys.path.append('../../mammut/')
import mammut

seconds = 10

m = mammut.Mammut()
e = m.getInstanceEnergy()
c = e.getCounter()

if c:
    startJoules = c.getJoules()
    time.sleep(seconds)
    endJoules = c.getJoules()

    print("Consumed " + str(endJoules - startJoules) + " in the last " + seconds + " seconds.")
else:
    print("Energy counters not available.")
# Last edited by Heraldo on 20140223

from pylab import *
from scipy import *

a,b,c,d = 0.1, 0.2, -65, 2     # Izhikevich model parameters Fast Spiking
maxTime = 50                   # maximum amount of time to run model in ms

dt = .5     # delta t (ms)
v = -70       # voltage variable, initial conditions
u = -14       # recovery variable, initial conditions

# things to store simulation data for plotting
hist_v = []
hist_u = []
hist_t = []
hist_I = []
spikeCount = 0

# at every time step...
for t in arange(int(maxTime/dt))*dt:
    
    # determine when to apply input current
    
    if t==33:
        I = -35
    elif t==35:
        I=-50
    else:
        I = 0

    
    
    # update model variables
            #  v += (0.04*v*v + 5*v + 140 - u + I)*dt  # determine voltage
            #  u += a*(b*v-u)*dt                       # determine recovery


    # update model variables
    v_old = v
    v += (0.04*v*v + 5*v + 140 - u)*dt  + I  # determine voltage
    u += a*(b*v_old-u)*dt                       # determine recovery
    
    # determine if a spike has occurred
    if v >= 30:
        # v = 30  # set the top of the spike
        # save a point for the top of the spike
        hist_v.append(v)
        hist_u.append(u)
        hist_t.append(t)
        hist_I.append(I)
        spikeCount += 1
        
        # reset variables as per Izhikevich model
        v = c
        u += d 
    
    # save a point
    hist_v.append(v)
    hist_u.append(u)
    hist_t.append(t)
    hist_I.append(I)
    
# convert histories to arrays for easier printing 
vv = array(hist_v)
uu = array(hist_u)
tt = array(hist_t)
II = array(hist_I)

print "total spikes:", spikeCount   # total # of spikes
plot( tt, vv )      # plot voltage (blue)
plot( tt, uu )      # plot recovery (green)
plot( tt, II )
xlabel('time (ms)')
ylabel('voltage (mV)')
show()
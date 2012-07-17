from pylab import *

x,mhdz,mhd0,mhd1,mhd2,mhd3,mhdgal3 = genfromtxt('coma_profile_dolag.csv',delimiter=',',unpack=True,skiprows=1)
plot(x,mhdz,c='b',label='Original')
#plot(x,mhd0,c='c',label='mhd0')
#plot(x,mhd1,c='b',label='mhd1')
#plot(x,mhd2,c='g',label='mhd2')
#plot(x,mhd3,c='r',label='mhd3')
#plot(x,mhdgal3,c='m',label='mhd_gal3')

ax,directWeighted = genfromtxt('coma_profile_volume_weighted.csv',delimiter=' ',unpack=True,skiprows=1)
#direct = (direct)/1e-6
ax = ax / 2693.1857
directWeighted = directWeighted/1e-6
#sampled = sampled/1e-6
#direct3 = direct*3
#sampled3 = sampled*3
#plot(ax,direct,'r',label='Reproduced')
plot(ax,directWeighted,'g--',label='Reproduced and Weighted')
#plot(ax,sampled,'b',label='sampled')
#plot(ax,direct3,'r--',label='direct * 3')
#plot(ax,sampled3,'b--',label='sampled * 3')

#ax2,direct2,sampled2 = genfromtxt('coma-1.0.txt',delimiter=' ',unpack=True,skiprows=1)
#direct2 = direct2/1e-6
#sampled2 = sampled2/1e-6
#direct32 = direct2*3
#sampled32 = sampled2*3
#plot(ax2,direct2,c='c',label='direct 1', marker='+')
#plot(ax2,sampled2,c='b',label='sampled 1', marker='+')
#plot(ax2,direct32,c='g',label='direct3 1', marker='+')
#plot(ax2,sampled32,c='r',label='sampled3 1', marker='+')




grid()
legend(loc='lower left', title='Coma profile (mhd_z)')
loglog()
xlim([2e-2,1])
ylim([1e-3,12])
xlabel(r'$r/r_{vir}$')
ylabel(r'$< | B | > [\mu G]$')
show()



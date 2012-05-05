from pylab import *

# plot original
x,mhdz,mhd0,mhd1,mhd2,mhd3,mhdgal3 = genfromtxt('test/coma_profile_dolag.csv',delimiter=',',unpack=True,skiprows=1)
plot(x,mhdz,c='b',label='Original')

# plot gadget
ax,directWeighted = genfromtxt('coma_profile_volume_weighted.csv',delimiter=' ',unpack=True,skiprows=1)
ax = ax / 2693.1857
directWeighted = directWeighted/1e-6
plot(ax,directWeighted,'g--',label='Reproduced and Weighted')

grid()
legend(loc='lower left', title='Coma profile (mhd_z)')
loglog()
xlim([2e-2,1])
ylim([1e-3,12])
xlabel(r'$r/r_{vir}$')
ylabel(r'$< | B | > [\mu G]$')
show()



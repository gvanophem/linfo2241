import numpy as np


def calcul(filename):
    f = open(filename, "r")
    sum = 0.0
    index = 0
    numbers = np.genfromtxt(filename)
    for i in numbers:
        sum+= float(i)
        index += 1
    return sum/index

def means():
    moy = []
    filenames = ["/etinfo/users/2021/gvanophem/linfo2241/num_threads/j1_s64_k16_r25.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j2_ref.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j5.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j10.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j15.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j20.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j25.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j30.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j40.txt", "/etinfo/users/2021/gvanophem/linfo2241/num_threads/j50.txt"]
    for i in filenames:
        moy.append(calcul(i))
    return moy

mean = means()
for i in mean:
    print(i)
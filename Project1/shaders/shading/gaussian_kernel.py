import numpy as np

def gaussian(u, sigma2):
    return np.exp(-(u*u)/(2*sigma2))/np.sqrt(2*np.pi*sigma2)

def kernel_1d(urange, sigma2):
    weight = 0
    for i in range(-urange, urange+1, 1):
        weight += gaussian(i, sigma2)
    wlist = []
    for i in range(0, urange+1, 1):
        wlist.append(gaussian(i, sigma2)/weight)
    return wlist

if __name__ == "__main__":
    wlist = []
    for i in range(15):
        wlist.extend(kernel_1d(i, i + 1))
    print(wlist)
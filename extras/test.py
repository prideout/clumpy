#!/usr/bin/env python3

import numpy as np
from PIL import Image
from os import system
from skimage import img_as_ubyte

def clumpy(cmd):
    system('./clumpy ' + cmd)

clumpy('generate_simplex 1000x500 1.0 4.0  0 noise1.npy')
clumpy('generate_simplex 1000x500 1.0 8.0  1 noise2.npy')
clumpy('generate_simplex 1000x500 1.0 16.0 2 noise3.npy')
clumpy('generate_simplex 1000x500 1.0 32.0 3 noise4.npy')
a, b, c, d = (np.load('noise{}.npy'.format(i)) for i in [1,2,3,4])
noise = a + b # + c + d

clumpy('generate_dshapes 1000x500 5 1234 shapes.npy')
shapes = np.clip(np.load('shapes.npy'), 0, 1)

sdf = np.clip(abs(noise) * shapes, 0, 1)
np.save('potential.npy', sdf)

if False:
    clumpy('visualize_sdf potential.npy viz.npy')
    Image.fromarray(np.load('viz.npy'), 'RGB').show()

clumpy('curl_2d potential.npy velocity.npy')
velocity = np.load('velocity.npy')
a = np.amin(velocity)
b = np.amax(velocity)
velocity = img_as_ubyte((velocity - a)/ (b - a))
red, grn = velocity
blu = img_as_ubyte(np.zeros(red.shape))
bands = [Image.fromarray(c) for c in [red, grn, blu]]
Image.merge('RGB', bands).save('redgrn.png')

# https://matplotlib.org/gallery/images_contours_and_fields/plot_streamplot.html
# https://scipython.com/blog/visualizing-a-vector-field-with-matplotlib/
def generate_plot():
    import matplotlib.pyplot as plt
    u, v = np.load('velocity.npy')
    x = np.linspace(-1, 1, 1000)
    y = np.linspace(-1, 1, 500)
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.streamplot(x, y, u, v, linewidth=1, density=1, arrowstyle='-', arrowsize=1)
    ax.xaxis.set_visible(False)
    ax.yaxis.set_visible(False)
    ax.set_aspect(0.5)
    plt.savefig('matplotlib.png', dpi=1000)

generate_plot()

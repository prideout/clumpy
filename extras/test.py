#!/usr/bin/env python3

import numpy as np
from PIL import Image
from os import system
from skimage import img_as_ubyte

def clumpy(cmd):
    system('./clumpy ' + cmd)

clumpy('generate_simplex 1024x512 1.0 4.0  0 noise1.npy')
clumpy('generate_simplex 1024x512 1.0 8.0  1 noise2.npy')
# clumpy('generate_simplex 1024x512 1.0 16.0 2 noise3.npy')
# clumpy('generate_simplex 1024x512 1.0 32.0 3 noise4.npy')
# a, b, c, d = (np.load('noise{}.npy'.format(i)) for i in [1,2,3,4])
# noise = a + b + c + d

a, b = (np.load('noise{}.npy'.format(i)) for i in [1,2])
noise = a + b

clumpy('generate_dshapes 1024x512 5 1234 shapes.npy')
shapes = np.clip(np.load('shapes.npy'), 0, 1)

sdf = np.clip(abs(noise) * shapes, 0, 1)
np.save('potential.npy', sdf)

clumpy('visualize_sdf potential.npy rgba viz.npy')
overlay_img = Image.fromarray(np.load('viz.npy'), 'RGBA')
overlay_img.save('overlay.png')

clumpy('curl_2d potential.npy velocity.npy')
velocity = np.load('velocity.npy').swapaxes(0, 2).swapaxes(1, 2)
a = np.amin(velocity)
b = np.amax(velocity)
velocity = img_as_ubyte((velocity - a)/ (b - a))
red, grn = velocity
blu = img_as_ubyte(np.zeros(red.shape))
bands = [Image.fromarray(c) for c in [red, grn, blu]]
Image.merge('RGB', bands).save('redgrn.png')

thickpts = False

if thickpts:
    clumpy('bridson_points 1024x512 5 42 pts.npy')
    clumpy('cull_points pts.npy potential.npy pts.npy')
    clumpy('advect_points pts.npy velocity.npy ' +
        '{step_size} {kernel_size} {decay} {nframes} anim.npy'.format(
            step_size = 300,
            kernel_size = 3,
            decay = 0.9,
            nframes = 240
        ))
else:
    clumpy('bridson_points 1024x512 5 0 pts.npy')
    clumpy('cull_points pts.npy potential.npy pts.npy')
    clumpy('advect_points pts.npy velocity.npy ' +
        '{step_size} {kernel_size} {decay} {nframes} anim.npy'.format(
            step_size = 399,
            kernel_size = 1,
            decay = 0.9,
            nframes = 240
        ))

import imageio
writer = imageio.get_writer('anim.mp4', fps=60)
opaque = Image.new('L', overlay_img.size, 255)
for i in range(240):
    filename = "{:03}anim.npy".format(i)
    frame_data = 255 - np.load(filename)
    base_img = Image.fromarray(frame_data, 'L')
    base_img = Image.merge('RGBA', [base_img, base_img, base_img, opaque])
    composited = Image.alpha_composite(base_img, overlay_img)
    frame_data = np.array(composited)
    writer.append_data(frame_data)
writer.close()
print('Generated anim.mp4')

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

# generate_plot()

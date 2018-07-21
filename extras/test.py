#!/usr/bin/env python3

import numpy as np
from PIL import Image
from os import system

def clumpy(cmd):
    result = system('./clumpy ' + cmd)
    if result: raise Exception("clumpy failed with: " + cmd)

NOISE_SCALE = 0.7
DISTANCE_SCALE = 0.5
LARGE_SPRITES = False
STEP_SIZE = 300
CREATE_REDGREEN_IMAGE = False
USE_MATPLOTLIB = False

clumpy('generate_simplex 1024x512 1.0 4.0  0 noise1.npy')
clumpy('generate_simplex 1024x512 1.0 8.0  1 noise2.npy')
a, b = (np.load('noise{}.npy'.format(i)) for i in [1,2])
noise = a + b

clumpy('generate_dshapes 1024x512 1 0 shapes.npy')
shapes = np.load('shapes.npy')
sdf = (NOISE_SCALE * noise + 1.0) * shapes * DISTANCE_SCALE
np.save('potential.npy', sdf)
clumpy('visualize_sdf shapes.npy rgba viz.npy')
overlay_img = Image.fromarray(np.load('viz.npy'), 'RGBA')
clumpy('curl_2d potential.npy velocity.npy')

if CREATE_REDGREEN_IMAGE:
    from skimage import img_as_ubyte
    velocity = np.load('velocity.npy').swapaxes(0, 2).swapaxes(1, 2)
    a = np.amin(velocity)
    b = np.amax(velocity)
    velocity = img_as_ubyte((velocity - a)/ (b - a))
    red, grn = velocity
    blu = img_as_ubyte(np.zeros(red.shape))
    bands = [Image.fromarray(c) for c in [red, grn, blu]]
    Image.merge('RGB', bands).save('redgrn.png')

if LARGE_SPRITES:
    clumpy('bridson_points 1024x512 15 0 pts.npy')
    clumpy('cull_points pts.npy potential.npy pts.npy')
    clumpy('advect_points pts.npy velocity.npy ' +
        '{step_size} {kernel_size} {decay} {nframes} anim.npy'.format(
            step_size = STEP_SIZE,
            kernel_size = 5,
            decay = 0.9,
            nframes = 240
        ))
else:
    clumpy('bridson_points 1024x512 5 0 pts.npy')
    clumpy('cull_points pts.npy potential.npy pts.npy')
    clumpy('advect_points pts.npy velocity.npy ' +
        '{step_size} {kernel_size} {decay} {nframes} anim.npy'.format(
            step_size = STEP_SIZE,
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
if USE_MATPLOTLIB:
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

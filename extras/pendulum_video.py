#!/usr/bin/env python3

import numpy as np
from os import system
import snowy
from tqdm import tqdm

result = system('cmake --build .release')
if result: raise Exception("build failed")

def clumpy(cmd):
    result = system('.release/clumpy ' + cmd)
    if result: raise Exception("clumpy failed with: " + cmd)

spacing = 25
step_size = 2.5
skip = 1
kernel_size = 7
decay = 0.99
nframes = 150
res = 1024, 1024
dim = 'x'.join(map(str,res))
friction = 0.8

# https://developer.twitter.com/en/docs/media/upload-media/uploading-media/media-best-practices.html
# < 512 MB,1280x720, bitrate=2048K
# H264 High Profile

if True:
    clumpy(f'pendulum_render {dim} {friction} {nframes} 20 5 render.npy')

    import imageio
    writer = imageio.get_writer('anim.mp4', fps=60, quality=9)
    for i in tqdm(range(0, nframes, skip)):
        im = np.load("{:03}render.npy".format(i))
        writer.append_data(np.uint8(im))

    writer.close()
    print('Generated anim.mp4')

if False:
    clumpy(f'pendulum_phase {dim} {friction} 1 5 field.npy')
    clumpy(f'bridson_points {dim} {spacing} 0 pts.npy')
    clumpy('advect_points pts.npy field.npy ' +
        f'{step_size} {kernel_size} {decay} {nframes} field1.npy')

    friction = 0.9
    clumpy(f'pendulum_phase {dim} {friction} 1 5 field.npy')
    clumpy(f'bridson_points {dim} {spacing} 0 pts.npy')
    clumpy('advect_points pts.npy field.npy ' +
        f'{step_size} {kernel_size} {decay} {nframes} field2.npy')

    import imageio
    writer = imageio.get_writer('anim.mp4', fps=60, quality=9)
    for i in tqdm(range(0, nframes, skip)):

        im1 = snowy.reshape(np.load("{:03}field1.npy".format(i)))
        im1 = 255.0 - 0.5 * snowy.resize(im1, 256, 256)

        im2 = snowy.reshape(np.load("{:03}field2.npy".format(i)))
        im2 = 255.0 - 0.5 * snowy.resize(im2, 256, 256)

        im = np.uint8(snowy.hstack([im1, im2], border_width=4))
        writer.append_data(im)

    writer.close()
    print('Generated anim.mp4')

system('rm *.npy')

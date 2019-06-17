#!/usr/bin/env python3

import numpy as np
from os import system
import snowy
from tqdm import tqdm

def clumpy(cmd):
    result = system('cmake --build .release')
    if result: raise Exception("build failed")
    result = system('.release/clumpy ' + cmd)
    if result: raise Exception("clumpy failed with: " + cmd)

spacing = 20
step_size = 2.5
skip = 2
kernel_size = 5
decay = 0.99
nframes = 240
res = 2000, 2000
dim = 'x'.join(map(str,res))

friction = 0.1
clumpy(f'simulate_pendulum field {dim} {friction} field.npy')
clumpy(f'bridson_points {dim} {spacing} 0 pts.npy')
clumpy('advect_points pts.npy field.npy ' +
    f'{step_size} {kernel_size} {decay} {nframes} anim1.npy')

friction = 0.9
clumpy(f'simulate_pendulum field {dim} {friction} field.npy')
clumpy(f'bridson_points {dim} {spacing} 0 pts.npy')
clumpy('advect_points pts.npy field.npy ' +
    f'{step_size} {kernel_size} {decay} {nframes} anim2.npy')

import imageio
writer = imageio.get_writer('anim.mp4', fps=60)
for i in tqdm(range(0, nframes, skip)):

    im1 = snowy.reshape(np.load("{:03}anim1.npy".format(i)))
    im1 = snowy.resize(im1, 256, 256)

    im2 = snowy.reshape(np.load("{:03}anim2.npy".format(i)))
    im2 = snowy.resize(im2, 256, 256)

    im = np.uint8(255.0 - snowy.hstack([im1, im2], border_width=4))
    writer.append_data(im)

writer.close()
print('Generated anim.mp4')

system('rm *.npy')

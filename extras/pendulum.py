#!/usr/bin/env python3

import numpy as np
from PIL import Image
from os import system

def clumpy(cmd):
    result = system('cmake --build .release')
    if result: raise Exception("build failed")
    result = system('.release/clumpy ' + cmd)
    if result: raise Exception("clumpy failed with: " + cmd)

friction = 1
step_size = 0.1
kernel_size = 1
decay = 1.0
nframes = 240

clumpy(f'simulate_pendulum 500x500 {friction} field.npy')
clumpy('bridson_points 500x500 5 0 pts.npy')
clumpy('advect_points pts.npy field.npy ' +
    f'{step_size} {kernel_size} {decay} {nframes} anim.npy')

import imageio
writer = imageio.get_writer('anim.mp4', fps=60)
opaque = Image.new('L', (500,500), 255)
for i in range(nframes):
    filename = "{:03}anim.npy".format(i)
    frame_data = 255 - np.load(filename)
    base_img = Image.fromarray(frame_data, 'L')
    base_img = Image.merge('RGBA', [base_img, base_img, base_img, opaque])
    frame_data = np.array(base_img)
    writer.append_data(frame_data)
writer.close()
print('Generated anim.mp4')

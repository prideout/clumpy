#!/usr/bin/env python3

import numpy as np
from PIL import Image
from os import system
from tqdm import tqdm

NOISE_SCALE = 0.7
DISTANCE_SCALE = 0.5
STEP_SIZE = 3000
WIDTH, HEIGHT = 2048, 1024
POINT_SIZE = 11
SPACING = 50

def clumpy(cmd, *args):
    args = cmd.format(*args)
    result = system('./clumpy ' + args)
    if result: raise Exception("clumpy failed with: " + args)

def clumpykw(cmd, **kwargs):
    args = cmd.format(**kwargs)
    result = system('./clumpy ' + args)
    if result: raise Exception("clumpy failed with: " + args)

# Generate noise.
print('\nGENERATE NOISE')
clumpy('generate_simplex {}x{} 1.0 4.0 0 noise1.npy', WIDTH, HEIGHT)
clumpy('generate_simplex {}x{} 1.0 8.0 1 noise2.npy', WIDTH, HEIGHT)
a, b = (np.load('noise{}.npy'.format(i)) for i in [1,2])
noise = a + b

# Generate distance field.
print('\nGENERATE DISTANCE FIELD')
clumpy('generate_dshapes {}x{} 1 0 shapes.npy', WIDTH, HEIGHT)
clumpy('visualize_sdf shapes.npy rgb distancefield.npy')
clumpy('visualize_sdf shapes.npy rgba overlay.npy')
distancefield = Image.fromarray(np.load('distancefield.npy'), 'RGB')
distancefield.save('distancefield.png')
overlayimg = Image.fromarray(np.load('overlay.npy'), 'RGBA')

# Multiply noise with distance and take the curl.
print('\nGENERATE VELOCITY')
shapes = np.load('shapes.npy')
sdf = (NOISE_SCALE * noise + 1.0) * shapes * DISTANCE_SCALE
np.save('potential.npy', sdf)
clumpy('curl_2d potential.npy velocity.npy')

# Make an animation.
print('\nMAKE ANIMATION')
clumpy('bridson_points {}x{} {} 0 pts.npy', WIDTH, HEIGHT, SPACING)
clumpy('cull_points pts.npy potential.npy pts.npy')
clumpykw('advect_points pts.npy velocity.npy {step} {ptsize} {decay} {nframes} anim.npy',
        step = STEP_SIZE,
        ptsize = POINT_SIZE,
        decay = 0.9,
        nframes = 240)

# Encode video.
print('\nENCODE VIDEO')
import imageio
writer = imageio.get_writer('anim.mp4', fps=60)
opaque = Image.new('L', overlayimg.size, 255)
for i in tqdm(range(240)):
    filename = "{:03}anim.npy".format(i)
    frame_data = 255 - np.load(filename)
    base_img = Image.fromarray(frame_data, 'L')
    base_img = Image.merge('RGBA', [base_img, base_img, base_img, opaque])
    composited = Image.alpha_composite(base_img, overlayimg)
    composited = composited.resize((512, 256), resample=Image.BILINEAR)
    frame_data = np.array(composited)
    writer.append_data(frame_data)
print('Closing stream...\n')
writer.close()
print('Generated anim.mp4')

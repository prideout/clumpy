#!/usr/bin/env python3

import numpy as np
import math
from PIL import Image
from os import system
from tqdm import tqdm

# import scipy.ndimage
# smallvel = scipy.ndimage.zoom(np.load('velocity.npy'), 0.5, order=3)
# np.save('smallvel.npy', smallvel)

NOISE_SCALE = 0.7
DISTANCE_SCALE = 0.5
STEP_SIZE = 3000
WIDTH, HEIGHT = 2048, 1024
BLOG_IMG_DIMS = (1024, 512)
VIDEO_DIMS = (512, 256)
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

def saveimg(im, filename):
    im = im.resize(BLOG_IMG_DIMS, resample=Image.BILINEAR)
    im.save(filename)
    system('optipng ' + filename)
    system('open ' + filename)

# Generate noise.
print('\nGENERATE NOISE')
clumpy('generate_simplex {}x{} 1.0 4.0 0 noise1.npy', WIDTH, HEIGHT)
clumpy('generate_simplex {}x{} 1.0 8.0 1 noise2.npy', WIDTH, HEIGHT)
a, b = (np.load('noise{}.npy'.format(i)) for i in [1,2])
noise = a + b
a, b = np.amin(noise), np.amax(noise)
viznoise = (noise - a) / (b - a)
im = Image.fromarray(np.uint8(255 * viznoise), 'L')
# saveimg(im, 'noise.png')

# Generate angle-based vector field and visualize it with a red-green image.
theta = viznoise * 2 * math.pi
red, grn = 0.5 + 0.5 * np.array([np.sin(theta), np.cos(theta)])
blu = np.zeros(red.shape)
bands = [np.uint8(255 * v) for v in (red, grn, blu)]
bands = [Image.fromarray(c) for c in bands]
im = Image.merge('RGB', bands)
# saveimg(im, 'angle_noise.png')

# Generate distance field and overlay image.
print('\nGENERATE DISTANCE FIELD')
clumpy('generate_dshapes {}x{} 1 0 shapes.npy', WIDTH, HEIGHT)
clumpy('visualize_sdf shapes.npy rgba overlay.npy')
overlayimg = Image.fromarray(np.load('overlay.npy'), 'RGBA')

# Visualize distance field.
clumpy('visualize_sdf shapes.npy rgb distancefield.npy')
im = Image.fromarray(np.load('distancefield.npy'), 'RGB')
# saveimg(im, 'distancefield.png')

# Multiply noise with distance and take the curl.
print('\nGENERATE VELOCITY')
shapes = np.load('shapes.npy')
sdf = (NOISE_SCALE * noise + 1.0) * shapes * DISTANCE_SCALE
np.save('potential.npy', sdf)
clumpy('curl_2d potential.npy velocity.npy')

# Use CGAL to render streamlines.
print('\nRENDER STREAMLINES')
clumpy('cgal_streamlines velocity.npy 5 1 streamlines.npy')
streamlines = np.load('streamlines.npy')
im = Image.fromarray(255 - streamlines, 'L')
A = Image.new('L', im.size, 255)
im = Image.merge('RGBA', [im, im, im, A])
composited = Image.alpha_composite(im, overlayimg)
saveimg(composited, 'streamlines.png')
quit()

# Visualize the curl-based noise with a red-green image.
vel = np.load('velocity.npy')
a, b = np.amin(vel), np.amax(vel)
vel = (vel - a) / (b - a)
red, grn = vel.swapaxes(0, 2).swapaxes(1, 2)
blu = np.zeros(red.shape)
bands = [np.uint8(255 * v) for v in (red, grn, blu)]
bands = [Image.fromarray(c) for c in bands]
im = Image.merge('RGB', bands)
saveimg(im, 'curl_noise.png')

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
A = Image.new('L', overlayimg.size, 255)
for i in tqdm(range(240)):
    filename = "{:03}anim.npy".format(i)
    frame_data = 255 - np.load(filename)
    base_img = Image.fromarray(frame_data, 'L')
    base_img = Image.merge('RGBA', [base_img, base_img, base_img, A])
    composited = Image.alpha_composite(base_img, overlayimg)
    if i == 0:
        saveimg(composited, 'screencap.png')
    composited = composited.resize(VIDEO_DIMS, resample=Image.BILINEAR)
    frame_data = np.array(composited)
    writer.append_data(frame_data)
print('Closing stream...\n')
writer.close()
system('open anim.mp4')

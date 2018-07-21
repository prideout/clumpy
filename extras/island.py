#!/usr/bin/env python3

# Pixel coordinates are [row,column] integers that increase right-down (like Raster Scans)
# Viewport coordinates are [x,y] floats that increase right-up (like Mathematics)

import numpy as np
import imageio
from os import system
from PIL import Image

Dims = [256, 256]
InitialFrequency = 1.0
Zoom = 0
shift = [-.2, -.1]
Viewport = [(-1+shift[0], -1+shift[1]), (+1+shift[0], +1+shift[1])]
TargetPt = [0, 0]
TargetLineSegment = ( [128,128], [0,100] ) # draw a line from center to top-right

def main():
    noise = gradient_noise(Dims, Viewport, InitialFrequency, seed=Zoom)
    TargetPt = marching_line(noise, TargetLineSegment)
    print("TargetPt =", TargetPt)
    L = np.uint8(255 * noise)
    L = np.array([L, L, L]).swapaxes(0, 2).swapaxes(0, 1)
    splat_point(L, TargetPt, [255, 0, 0])
    img = Image.fromarray(L, 'RGB')
    img = img.resize((1024, 1024), Image.NEAREST)
    img.save('gradient_noise.png')

def splat_point(rgb_array, pxcoord, rgb):
    rgb_array[pxcoord[0]][pxcoord[1]] = rgb

def clumpy(cmd):
    result = system('./clumpy ' + cmd)
    if result: raise Exception("clumpy failed with: " + cmd)

def gradient_noise(dims, viewport, frequency, seed):
    args = "{}x{} '{},{},{},{}' {} {}".format(
        dims[0], dims[1],
        viewport[0][0], viewport[0][1], viewport[1][0], viewport[1][1],
        frequency, seed)
    clumpy("gradient_noise " + args)
    return np.load('gradient_noise.npy')

def make_video(arrays, outfile = 'out.mp4'):
    writer = imageio.get_writer(outfile, fps=30)
    for array in arrays:
        writer.append_data(array)
    writer.close()

def marching_line(image_array, line_segment):
    x0, y0 = line_segment[0]
    x1, y1 = line_segment[1]
    i, j = int(x0), int(y0)
    val = image_array[i][j]
    sgn = np.sign(val)
    divs = max(Dims)
    dx = float(x1 - x0) / float(divs)
    dy = float(y1 - y0) / float(divs)
    for _ in range(divs):
        x = float(x0) + float(_) * dx
        y = float(y0) + float(_) * dy
        i, j = int(x), int(y)
        val = image_array[i][j]
        if np.sign(val) != sgn:
            return [i, j]
    return [-1, -1]

main()

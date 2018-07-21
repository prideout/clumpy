#!/usr/bin/env python3

'''
Creates a movie of infinitely zooming FBM.

Pixel coordinates are [row,column] and increase right-down (like Raster Scans)
Viewport coordinates are [x,y] and increase right-up (like Mathematics)
'''

import numpy as np
import imageio
import time
import math
import cairo
from os import system
from tqdm import tqdm

Dims = np.uint16([512, 512])
Lut = np.float32(np.zeros([256, 3]))
InitialFrequency = 1.0
NumOctaves = 4
NumFrames = 60

# Define an intersection line in pixel coordinates
TargetLineSegment = ( Dims / 2.0, [0, Dims[1] * 0.75] )

Zoom = 0 # used only as a seed for randomness
Viewport = np.float32([(-1, -1), (+1, +1)])
TargetPt = [0, 0]
HeightMap = np.zeros(Dims)
AccumMap = np.zeros(Dims)

def update_heightmap():
    global HeightMap
    frequency = InitialFrequency
    HeightMap = np.copy(AccumMap)
    for i in range(NumOctaves):
        HeightMap += gradient_noise(Dims, Viewport, frequency, seed=Zoom+i)
        HeightMap *= 2
        frequency *= 2

def main():
    global TargetPt
    update_heightmap()
    TargetPt = marching_line(HeightMap, TargetLineSegment)
    writer = imageio.get_writer('out.mp4', fps=30)
    for frame in tqdm(range(NumFrames)):
        update_heightmap()
        # TargetPt = marching_line(HeightMap, TargetLineSegment)
        rgb = render_heightmap()
        writer.append_data(np.uint8(rgb))
        shrink_viewport()
    writer.close()

def render_heightmap():
    L = np.uint8(255 * (0.5 + HeightMap * 0.5))
    L = Lut[L]
    draw_overlay(L, TargetLineSegment, TargetPt)
    return L

def shrink_viewport():
    global Viewport
    global TargetLineSegment
    global TargetPt
    row, col = TargetPt
    (left, bottom), (right, top) = Viewport
    vpwidth = right - left
    vpheight = top - bottom
    x = left + vpwidth * float(col) / Dims[0]
    y = top - vpheight * float(row) / Dims[1]
    dx = vpwidth / float(Dims[0])
    dy = vpheight / float(Dims[1])
    M = min(dx, dy)
    cx = (left + right) * 0.5
    cy = (bottom + top) * 0.5
    x -= cx
    y -= cy
    if abs(x) < M:
        left += dx
        right -= dx
    elif x < 0:
        right -= dx * 2
    else:
        left += dx * 2
    if abs(y) < M:
        bottom += dy
        top -= dy
    elif y < 0:
        top -= dy * 2
    else:
        bottom += dy * 2
    x += cx
    y += cy
    # TODO: transform TargetLineSegment rather than TargetPt
    Viewport = [(left, bottom), (right, top)]
    vpwidth = right - left
    vpheight = top - bottom
    col = (x - left) * Dims[0] / vpwidth
    row = (top - y) * Dims[1] / vpheight
    TargetPt = [row, col]

def draw_overlay(dst, lineseg, pxcoord):
    dims = dst.shape
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, dims[0], dims[1])
    ctx = cairo.Context(surface)
    ctx.set_line_width(dims[0] / 150.0)
    # Stroke a path along lineseg
    ctx.set_source_rgba(1.0, 0.8, 0.8, 0.8)
    ctx.move_to(lineseg[0][1], lineseg[0][0])
    ctx.line_to(lineseg[1][1], lineseg[1][0])
    ctx.stroke()
    # Draw circle around pxcoord
    ctx.set_source_rgba(1.0, 0.8, 0.8)
    ctx.save()
    ctx.translate(pxcoord[1], pxcoord[0])
    ctx.scale(dims[0] / 60.0, dims[1] / 60.0)
    ctx.arc(0., 0., 1., 0., 2 * math.pi)
    ctx.restore()
    ctx.stroke()
    if False:
        f = cairo.ToyFontFace("")
        ctx.move_to(200, 200)
        ctx.show_text("Philip")
    # Perform composition
    buf = surface.get_data()
    rgb = np.ndarray(shape=dims[:2], dtype=np.uint32, buffer=buf)
    color = np.float32([(rgb >> 8) & 0xff, (rgb >> 16) & 0xff, (rgb >> 0) & 0xff])
    color = color.swapaxes(0, 2).swapaxes(0, 1)
    a = np.float32((rgb >> 24) & 0xff) / 255.0
    alpha = np.array([a, a, a]).swapaxes(0, 2).swapaxes(0, 1)
    np.copyto(dst, dst * (1 - alpha) + color * alpha)

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

# TODO: Look at bottom for a nicer gradient.
def create_gradient():
    r = np.hstack([np.linspace(0.0,     0.0, num=128), np.linspace(0.0,     0.0, num=128)])
    g = np.hstack([np.linspace(0.0,     0.0, num=128), np.linspace(128.0, 255.0, num=128)])
    b = np.hstack([np.linspace(128.0, 255.0, num=128), np.linspace(0.0,    64.0, num=128)])
    lut = np.float32([r, g, b]).transpose()
    np.copyto(Lut, lut)
    '''
    Gradient = [
        000, 0x001070, # Dark Blue
        126, 0x2C5A7C, # Light Blue
        127, 0xE0F0A0, # Yellow
        128, 0x5D943C, # Dark Green
        160, 0x606011, # Brown
        200, 0xFFFFFF, # White
        255, 0xFFFFFF, # White
    ]
    '''

create_gradient()
main()

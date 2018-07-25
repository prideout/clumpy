#!/usr/bin/env python3

'''
Creates a movie of infinitely zooming FBM.

All coordinates are X,Y floats with values that increase rightward and downward:

    World Space: [0,0 through 1,1] spans the entire island.
     Tile Space: [0,0 through 1,1] spans the smallest tile that wholly encompasses the viewport.
 Viewport Space: [0,0 through 1,1] spans the current view.

At Z = 0, Tile Space is equivalent to World Space.

Note that numpy requires Row,Col integer coordinates, but we internalize those at the lowest level.
(see sample_pixel)

'''

from os import system
from tqdm import tqdm
from sdl2.ext import clipline

import cairo
import imageio
import numpy as np
import scipy.interpolate as interp

def vec2(x, y): return np.array([x, y], dtype=np.float)
def vec3(x, y, z): return np.array([x, y, z], dtype=np.float)
def grid(w, h): return np.zeros([int(h), int(w)], dtype=np.float)

# Configuration.
Resolution = vec2(512,512)
NoiseFrequency = 16.0
NumLayers = 4
VideoFps = 30
NumFrames = VideoFps * 10
vsTargetLn = vec2([.5,.5], [.75,0])
vsPanFocus = vec2(0.6, 0.4)
SeaLevel = 0.5
NicePalette = [
    000, 0x001070 , # Dark Blue
    126, 0x2C5A7C , # Light Blue
    127, 0xE0F0A0 , # Yellow
    128, 0x5D943C , # Dark Green
    160, 0x606011 , # Brown
    200, 0xFFFFFF , # White
    255, 0xFFFFFF ] # White

# Global data.
Width, Height = Resolution
Lut = grid(3, 256)
Zoom = int(0)
vsTargetPt = vec2(-1,-1)
ViewImage = grid(Width, Height) ## Current viewport. Includes NumLayers of high-freq noise.
TileImage = grid(Width, Height) ## Smallest encompassing tile (accumulated low-freq noise).
Viewports = []

def update_view(nlayers = NumLayers):
    resample_image(ViewImage, TileImage, Viewports[-1])
    seed = Zoom
    for vp in Viewports[:nlayers]:
        noise = gradient_noise(Resolution, vp, NoiseFrequency, seed=seed)
        np.copyto(ViewImage, 2 * (ViewImage + noise))
        seed = seed + 1

def update_tile():
    global Zoom
    global Viewports
    # Render a new base tile by adding one layer of noise.
    update_view(1)
    np.copyto(TileImage, ViewImage)
    # Left-shift the viewports array and push on a new high-frequency layer.
    Viewports = Viewports[1:] + [vec2((0,0),(1,1))]
    Zoom = Zoom + 1

def main():
    global vsTargetPt
    update_view()
    vsTargetPt = marching_line(ViewImage, vsTargetLn)
    writer = imageio.get_writer('out.mp4', fps=VideoFps, quality=9)
    for frame in tqdm(range(NumFrames)):

        # Draw the heightmap for the current viewport.
        update_view()

        # Recompute the point of interest.
        vsTargetPt = marching_line(ViewImage, vsTargetLn)

        # Draw the overlay and convert the heightmap into color.
        rgb = render_view()
        writer.append_data(np.uint8(rgb))

        # Compute the pan / zoom adjustments for the largest viewport.
        vpdelta = shrink_viewport(Viewports[-1], zoom_speed=10, pan_speed=0.05)

        # Propagate the movement to all layer viewports.
        for vp in reversed(Viewports):
            np.copyto(vp, vp + vpdelta)
            vpdelta = vpdelta / 2

        # If the largest viewport is sufficiently small, it's time to increment zoom.
        vp = Viewports[-1]
        vpextent = vp[1] - vp[0]
        if vpextent[0] < 0.5 and vpextent[1] < 0.5:
            update_tile()

    writer.close()

def render_view():
    clamped = np.clip(0.5 + 0.01 * ViewImage, 0, 1)
    L = np.uint8(255 * clamped)
    L = Lut[L]
    draw_overlay(L, vsTargetLn, vsTargetPt)
    return L

def shrink_viewport(viewport, zoom_speed, pan_speed):
    vpextent = viewport[1] - viewport[0]
    pandir = vsTargetPt - vsPanFocus
    pan_delta = pan_speed * pandir
    zoom_delta = zoom_speed * vpextent / Resolution
    return pan_delta + vec2(zoom_delta, -zoom_delta)

def draw_overlay(dst, lineseg, pxcoord):
    dims = [dst.shape[1], dst.shape[0]]
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, dims[0], dims[1])
    ctx = cairo.Context(surface)
    ctx.scale(dims[0], dims[1])
    ctx.set_line_width(0.005)
    # Stroke a path along lineseg
    ctx.set_source_rgba(1.0, 0.8, 0.8, 0.1)
    ctx.move_to(lineseg[0][0], lineseg[0][1])
    ctx.line_to(lineseg[1][0], lineseg[1][1])
    ctx.stroke()
    # Draw circle around pxcoord
    ctx.set_source_rgba(1.0, 0.8, 0.8, 1.0)
    ctx.save()
    ctx.translate(pxcoord[0], pxcoord[1])
    ctx.scale(0.02, 0.02)
    ctx.arc(0., 0., 1., 0., 2 * np.pi)
    ctx.restore()
    ctx.stroke()
    if False:
        f = cairo.ToyFontFace("")
        ctx.move_to(.5, .5)
        ctx.show_text("Philip")
    # Perform composition
    buf = surface.get_data()
    rgb = np.ndarray(shape=dims[:2], dtype=np.uint32, buffer=buf)
    color = np.float32([(rgb >> 8) & 0xff, (rgb >> 16) & 0xff, (rgb >> 0) & 0xff])
    color = color.swapaxes(0, 2).swapaxes(0, 1)
    a = np.float32((rgb >> 24) & 0xff) / 255.0
    alpha = np.array([a, a, a]).swapaxes(0, 2).swapaxes(0, 1)
    np.copyto(dst, dst * (1 - alpha) + color)

def clumpy(cmd):
    result = system('./clumpy ' + cmd)
    if result: raise Exception("clumpy failed with: " + cmd)

def gradient_noise(dims, viewport, frequency, seed):
    (left, top), (right, bottom) = 2 * (viewport - 0.5)
    args = "{}x{} '{},{},{},{}' {} {}".format(
        dims[0], dims[1],
        left, -bottom, right, -top,
        frequency, seed)
    clumpy("gradient_noise " + args)
    return np.load('gradient_noise.npy')

def sample_pixel(image_array, x, y):
    rows, cols = image_array.shape
    row, col = int(y * rows), int(x * cols)
    if row < 0 or col < 0 or col >= cols or row >= rows:
        return 0
    return image_array[row][col]

def marching_line(image_array, line_segment):
    x0, y0 = line_segment[0]
    x1, y1 = line_segment[1]
    val = sample_pixel(image_array, x0, y0)
    sgn = np.sign(val)
    divs = float(max(Resolution))
    dx = (x1 - x0) / divs
    dy = (y1 - y0) / divs
    for i in range(int(divs)):
        x = x0 + i * dx
        y = y0 + i * dy
        val = sample_pixel(image_array, x, y)
        if np.sign(val) != sgn:
            return [x, y]
    print(f'Could not find in {x0:3.3},{y0:3.3} -- {x1:3.3},{y1:3.3}')
    return [0.5,0.5]

# TODO: use NicePalette
def create_palette():
    r = np.hstack([np.linspace(0.0,     0.0, num=128), np.linspace(0.0,     0.0, num=128)])
    g = np.hstack([np.linspace(0.0,     0.0, num=128), np.linspace(128.0, 255.0, num=128)])
    b = np.hstack([np.linspace(128.0, 255.0, num=128), np.linspace(0.0,    64.0, num=128)])
    return np.float32([r, g, b]).transpose()

# Hermite interpolation, also known as smoothstep:
#     (-1 => 0)     (0 => 1)     (+1 => 0)
def hermite(t):
    return 1 - (3 - 2*np.abs(t))*t*t

def create_basetile(width, height):
    rows = hermite([np.linspace(-1.0, 1.0, num=height)])
    cols = hermite([np.linspace(-1.0, 1.0, num=width)]).T
    return rows * cols - SeaLevel

def resample_image(dst, src, viewport):
    height, width = dst.shape
    domain = [np.linspace(0, 1, num) for num in (width, height)]
    [(left, top), (right, bottom)] = viewport
    vrange = np.linspace(left, right, num=width)
    urange = np.linspace(top, bottom, num=height)
    f = interp.interp1d(domain[0], src, kind='linear')
    temp = f(vrange)
    f = interp.interp1d(domain[1], temp.T, kind='linear')
    newimg = f(urange).T
    np.copyto(dst, newimg)

# The largest viewport (highest frequency noise) is (0,0) - (1,1).
frequency = 1
for x in range(NumLayers):
    vp = vec2((0,0), (frequency,frequency))
    Viewports.insert(0, vp)
    frequency = frequency / 2

np.copyto(Lut, create_palette())
np.copyto(TileImage, create_basetile(Width, Height))
main()

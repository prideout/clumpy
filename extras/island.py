#!/usr/bin/env python3

'''
Creates a movie of infinitely zooming FBM.

All coordinates are X,Y floats with values that increase rightward and downward.

In contrast to this, numpy requires Row,Col integer coordinates, so we internalize those at the
lowest level. (see sample_pixel)

All coordinates live in one of these coordinate systems:

    World Space: [0,0 through 1,1] spans the entire island.
     Tile Space: [0,0 through 1,1] spans the smallest tile that wholly encompasses the viewport.
 Viewport Space: [0,0 through 1,1] spans the current view.

At Z = 0, Tile Space is equivalent to World Space.

'''

from os import system
from tqdm import tqdm

import cairo
import imageio
import numpy as np
import scipy.interpolate as interp

def vec2(x, y): return np.array([x, y])
def vec3(x, y, z): return np.array([x, y, z])

# Global configuration.
Resolution = [512,512]
Lut = np.zeros([256,3], dtype=np.float)
InitialFrequency = 1.0
NumOctaves = 4
NumFrames = 60
wsTargetLn = ([.5,.5], [.75,0])

# Global variables.
Zoom = int(0)
tsTargetLn = np.float32(wsTargetLn)
tsViewport = np.float32([(0,0), (1,1)])
tsTargetPt = [-1,-1]
Shape = [Resolution[1], Resolution[0]]  ## Numpy wants Rows,Cols rather than Width,Height.
ViewImage = np.zeros(Shape) ## Current viewport. Includes NumOctaves of high-freq noise.
TileImage = np.zeros(Shape) ## Smallest encompassing tile (accumulated low-freq noise).
Domain = [np.linspace(0, 1, num) for num in Resolution]

def resample_image(dst, src, viewport):
    [(left, top), (right, bottom)] = viewport
    width, height = Resolution
    urange = np.linspace(left, right, num=width)
    vrange = np.linspace(top, bottom, num=height)
    f = interp.interp1d(Domain[0], src, kind='linear')
    temp = f(vrange)
    f = interp.interp1d(Domain[1], temp.T, kind='linear')
    newimg = f(urange)
    np.copyto(dst, newimg)

def update_view():
    frequency = InitialFrequency
    resample_image(ViewImage, TileImage, tsViewport)
    for i in range(NumOctaves):
        noise = gradient_noise(Resolution, tsViewport, frequency, seed=Zoom+i)
        np.copyto(ViewImage, 2 * (ViewImage + noise))
        frequency *= 2

def main():
    global tsTargetPt
    update_view()
    tsTargetPt = marching_line(ViewImage, tsTargetLn)
    writer = imageio.get_writer('out.mp4', fps=30, quality=9)
    for frame in tqdm(range(NumFrames)):
        update_view()
        rgb = render_view()
        writer.append_data(np.uint8(rgb))
        shrink_viewport(zoom_speed=10, pan_speed=0.05)
    writer.close()

def render_view():
    lo, hi = np.amin(ViewImage), np.amax(ViewImage)
    L = np.uint8(255 * (0.5 + 0.5 * ViewImage / (hi - lo)))
    L = Lut[L]
    vpextent = tsViewport[1] - tsViewport[0]
    vsTargetPt = (tsTargetPt - tsViewport[0]) / vpextent
    vsTargetLn = [
        (tsTargetLn[0] - tsViewport[0]) / vpextent ,
        (tsTargetLn[1] - tsViewport[0]) / vpextent ]
    draw_overlay(L, vsTargetLn, vsTargetPt)
    return L

def shrink_viewport(zoom_speed, pan_speed):
    global tsViewport
    (left, top), (right, bottom) = tsViewport
    vpextent = tsViewport[1] - tsViewport[0]
    delta = zoom_speed * vpextent / Resolution
    vsTargetPt = (tsTargetPt - tsViewport[0]) / vpextent
    pandir = vsTargetPt - vec2(0.5, 0.5)
    pan = pan_speed * pandir
    tsViewport[0] += pan
    tsViewport[1] += pan
    # tsViewport += vec2(delta, -delta)
    tsViewport[0] += delta
    tsViewport[1] -= delta

def draw_overlay(dst, lineseg, pxcoord):
    dims = [dst.shape[1], dst.shape[0]]
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, dims[0], dims[1])
    ctx = cairo.Context(surface)
    ctx.scale(dims[0], dims[1])
    ctx.set_line_width(0.005)
    # Stroke a path along lineseg
    ctx.set_source_rgba(1.0, 0.8, 0.8, 0.8)
    ctx.move_to(lineseg[0][0], lineseg[0][1])
    ctx.line_to(lineseg[1][0], lineseg[1][1])
    ctx.stroke()
    # Draw circle around pxcoord
    ctx.set_source_rgba(1.0, 0.8, 0.8)
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
    (left, top), (right, bottom) = 2 * (tsViewport - 0.5)
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
    print('FAIL')
    return [-1, -1]

# TODO: use NiceGradient
def create_gradient():
    NiceGradient = [
        000, 0x001070, # Dark Blue
        126, 0x2C5A7C, # Light Blue
        127, 0xE0F0A0, # Yellow
        128, 0x5D943C, # Dark Green
        160, 0x606011, # Brown
        200, 0xFFFFFF, # White
        255, 0xFFFFFF, # White
    ]
    r = np.hstack([np.linspace(0.0,     0.0, num=128), np.linspace(0.0,     0.0, num=128)])
    g = np.hstack([np.linspace(0.0,     0.0, num=128), np.linspace(128.0, 255.0, num=128)])
    b = np.hstack([np.linspace(128.0, 255.0, num=128), np.linspace(0.0,    64.0, num=128)])
    lut = np.float32([r, g, b]).transpose()
    np.copyto(Lut, lut)

# Hermite interpolation, also known as smoothstep:
#     -1 => 0
#      0 => 1
#     +1 => 0
def hermite(t):
    return 1 - (3 - 2*np.abs(t))*t*t

def create_basetile():
    rows, cols = Shape
    rows = hermite([np.linspace(-1.0, 1.0, num=rows)])
    cols = hermite([np.linspace(-1.0, 1.0, num=cols)]).T
    kernel = rows * cols - 0.5
    np.copyto(TileImage, kernel)

create_gradient()
create_basetile()
main()

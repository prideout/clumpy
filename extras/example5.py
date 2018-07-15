#!/usr/bin/env python3

from numpy import load
from PIL import Image
from os import system

def clumpy(cmd):
    result = system('./clumpy ' + cmd)
    if result: raise Exception("clumpy failed with: " + cmd)

clumpy('generate_simplex 1000x500 1.0 8.0 0 potential.npy')
clumpy('curl_2d potential.npy velocity.npy')
clumpy('bridson_points 1000x500 5 0 pts.npy')
clumpy('advect_points pts.npy velocity.npy 30 1 0.95 240 anim.npy')
Image.fromarray(load("000anim.npy"), "L").point(lambda p: p * 2).show()

'''
def advect_points(**kwargs):
    cmd = 'input_points velocities step_size kernel_size decay nframes output_points'
    cmd = ' '.join(str(kwargs[arg]) for arg in cmd.split())
    return clumpy('advect_points ' + cmd)

advect_points(
    input_points='pts.npy',
    velocities='velocity.npy',
    step_size=30,
    kernel_size=1,
    decay=0.95,
    nframes=240,
    output_points='anim.npy')
'''

Image.fromarray(load("000anim.npy"), "L").point(lambda p: p * 2).save('example5.png')
system('optipng example5.png')

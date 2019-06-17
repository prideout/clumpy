#!/usr/bin/env python3

import numpy as np
from os import system
import snowy
from tqdm import tqdm

result = system("cmake --build .release")
if result: raise Exception("build failed")

def clumpy(cmd):
    print(f"clumpy {cmd}")
    result = system(f".release/clumpy {cmd}")
    if result: raise Exception("clumpy failed")

friction = 0.9
spacing = 20
step_size = 2.5
kernel_size = 5
decay = 0.99
nframes = 400
res = 4000, 2000
scalex = 2
scaley = 5

dim = "x".join(map(str,res))

clumpy(f"pendulum_phase {dim} {friction} {scalex} {scaley} field.npy")
clumpy(f"bridson_points {dim} {spacing} 0 pts.npy")
clumpy(f"advect_points pts.npy field.npy {step_size} {kernel_size} {decay} {nframes} phase.npy")

im = snowy.reshape(1.0 - np.load("000phase.npy") / 255.0)
im = snowy.resize(im, 500, 250)
snowy.export(im, "extras/example6.png")
snowy.show(im)
system("rm *.npy")
print("Generated extras/example6.png")

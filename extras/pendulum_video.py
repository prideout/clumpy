#!/usr/bin/env python3

import numpy as np
from os import system
import snowy
from tqdm import tqdm

result = system("cmake --build .release")
if result: raise Exception("build failed")

def clumpy(cmd):
    result = system(".release/clumpy " + cmd)
    if result: raise Exception("clumpy failed with: " + cmd)

spacing = 20
step_size = 2.5
skip = 1
kernel_size = 7
decay = 0.99
nframes = 240
res = 512, 512
dim = "x".join(map(str,res))

friction = 0.1
clumpy(f"pendulum_phase {dim} {friction} 1 5 field.npy")

if True:
    clumpy(f"pendulum_render field.npy {dim} render.npy")

    im = snowy.reshape(np.load("render.npy"))
    snowy.export(im, "render.png")

else:
    clumpy(f"bridson_points {dim} {spacing} 0 pts.npy")
    clumpy(f"advect_points pts.npy field.npy {step_size} {kernel_size} {decay} {nframes} field1.npy")

    friction = 0.9
    clumpy(f"pendulum_phase {dim} {friction} 1 5 field.npy")
    clumpy(f"bridson_points {dim} {spacing} 0 pts.npy")
    clumpy(f"advect_points pts.npy field.npy {step_size} {kernel_size} {decay} {nframes} field2.npy")

    import imageio
    writer = imageio.get_writer("anim.mp4", fps=60)
    for i in tqdm(range(0, nframes, skip)):

        im1 = snowy.reshape(np.load("{:03}field1.npy".format(i)))
        im1 = 255.0 - 0.5 * snowy.resize(im1, 256, 256)
        im1 = snowy.blur(im1, radius=4)

        im2 = snowy.reshape(np.load("{:03}field2.npy".format(i)))
        im2 = 255.0 - 0.5 * snowy.resize(im2, 256, 256)
        im2 = snowy.blur(im2, radius=4)

        im = np.uint8(snowy.hstack([im1, im2], border_width=4))
        writer.append_data(im)

    writer.close()
    print("Generated anim.mp4")

system("rm *.npy")

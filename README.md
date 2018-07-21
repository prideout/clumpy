[![Build Status](https://travis-ci.org/prideout/clumpy.svg?branch=master)](https://travis-ci.org/prideout/clumpy)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/prideout/clumpy/blob/master/LICENSE)

This tool can manipulate or generate large swaths of image data stored in [numpy
files](https://docs.scipy.org/doc/numpy/neps/npy-format.html). It's a sandbox for implementing
operations in C++ that are either slow or non-existent in [pillow](https://python-pillow.org/),
[scikit-image](http://scikit-image.org/), or the [SciPy](https://www.scipy.org/) ecosystem.

Since it's just a command line tool, it doesn't contain any
[FFI](https://en.wikipedia.org/wiki/Foreign_function_interface) messiness. Feel free to contribute
by adding your own command, but keep it simple! Add a `cc` file and make a pull request.

This is really just a toy library; for serious application you might want to look at
[xtensor](https://github.com/QuantStack/xtensor) (which can read / write npy files) and
[xtensor-io](https://github.com/QuantStack/xtensor-io).

---

Build and run clumpy.

    cmake -H. -B.release -GNinja && cmake --build .release
    alias clumpy=$PWD/.release/clumpy
    clumpy help

---

Generate two octaves of simplex noise and combine them.

    clumpy generate_simplex 500x250 0.5 16.0 0 noise1.npy
    clumpy generate_simplex 500x250 1.0 8.0  0 noise2.npy

    python <<EOL
    import numpy as np; from PIL import Image
    noise1, noise2 = np.load("noise1.npy"), np.load("noise2.npy")
    result = np.clip(np.abs(noise1 + noise2), 0, 1)
    Image.fromarray(np.uint8(result * 255), "L").show()
    EOL

<img src="https://github.com/prideout/clumpy/raw/master/extras/example1.png">

---

Create a distance field with a random shape.

    clumpy generate_dshapes 500x250 1 0 shapes.npy
    clumpy visualize_sdf shapes.npy rgb shapeviz.npy

    python <<EOL
    import numpy as np; from PIL import Image
    Image.fromarray(np.load('shapeviz.npy'), 'RGB').show()
    EOL

<img src="https://github.com/prideout/clumpy/raw/master/extras/example2.png">

---

Create a 2x2 atlas of distance fields, each with 5 random shapes.

    for i in {1..4}; do clumpy generate_dshapes 250x125 5 $i shapes$i.npy; done
    for i in {1..4}; do clumpy visualize_sdf shapes$i.npy shapes$i.npy; done
    
    python <<EOL
    import numpy as np; from PIL import Image
    a, b, c, d = (np.load('shapes{}.npy'.format(i)) for i in [1,2,3,4])
    img = np.vstack(((np.hstack((a,b)), np.hstack((c,d)))))
    Image.fromarray(img, 'RGB').show()
    EOL

<img src="https://github.com/prideout/clumpy/raw/master/extras/example3.png">

---

Create a nice distribution of ~20k points, cull points that overlap certain areas, and plot them. Do
all this in less than a second and use only one thread.

    clumpy bridson_points 500x250 2 0 coords.npy
    clumpy generate_dshapes 500x250 1 0 shapes.npy
    clumpy cull_points coords.npy shapes.npy culled.npy
    clumpy splat_points culled.npy 500x250 u8disk 1 1.0 splats.npy

    python <<EOL
    import numpy as np; from PIL import Image
    Image.fromarray(np.load("splats.npy"), "L").show()
    EOL

<img src="https://github.com/prideout/clumpy/raw/master/extras/example4.png">

---

You may wish to invoke clumpy from within Python using `os.system` or `subprocess.Popen `.

Here's an example that generates 240 frames of an advection animation with ~12k points, then
brightens up the last frame and displays it. This entire script takes about 1 second to execute and
uses only one core (3.1 GHz Intel Core i7).

```python
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
```

<img src="https://github.com/prideout/clumpy/raw/master/extras/example5.png">

<!--

TODO

Heman examples, then Deprecate Heman (add to README, then "Archive")

cnpy.h should be abstracted out into a base class methods for save and load.

appveyer windows build, like:
    https://t.co/bkJ7ZqXAGy

rename extras to docs then add a mkdocs pipeline

heman color island but without lighting
    shouldn't require any new functionality
    could be a python-first example.
    # Should this function throw if system returns nonzero?
    def clumpy(cmd):
        os.system('./clumpy ' + cmd)
    search for  "color lookup" here:
        https://docs.scipy.org/doc/numpy-1.12.0/user/basics.indexing.html
    look at pillow example here (although it should have h=1, then resize)
        https://stackoverflow.com/questions/25668828/how-to-create-colour-gradient-in-python
    clumpy('advect_points pts.npy velocity.npy ' +
        '{step_size} {kernel_size} {decay} {nframes} anim.npy'.format(
            step_size = 399,
            kernel_size = 1,
            decay = 0.9,
            nframes = 240
        ))

grayscale island waves sequence
    could perhaps use multiprocessing
    https://github.com/prideout/reba-island

lighting / AO...  make the streamlines look like 3D tadpoles?

find_contours <input_img> <output_svg>
    https://github.com/adishavit/simple-svg
    * int find_blobs( int16_t roi_x, int16_t roi_y, int16_t roi_w, int16_t roi_h,
    *                 uint8_t *in, int16_t in_w, int16_t in_h, 
    *                 label_t **label, int16_t *label_w, int16_t *label_h, 
    *                 blob_t** blobs, int *count, int extract_internal );

"Import a bitmap, generate a distance field from it, add noise, and export."

variable_blur
    https://github.com/scipy/scipy/blob/master/scipy/ndimage/filters.py#L213

gradient_magnitude (similar to curl2d)
    https://docs.scipy.org/doc/numpy/reference/routines.math.html

https://blind.guru/simple_cxx11_workqueue.html
    
-->

This tool can manipulate or generate large swaths of image data stored in [numpy
files](https://docs.scipy.org/doc/numpy/neps/npy-format.html). It's a sandbox for implementing
operations in C++ that are either slow or non-existent in [pillow](https://python-pillow.org/),
[scikit-image](http://scikit-image.org/), or the [SciPy](https://www.scipy.org/) ecosystem.

Since it's just a command line tool, it doesn't contain any
[FFI](https://en.wikipedia.org/wiki/Foreign_function_interface) messiness. Feel free to contribute
by adding your own command, but keep it simple! Add a `cc` file and make a pull request.

Build and run clumpy:

    cmake -H. -B.release -GNinja && cmake --build .release
    alias clumpy=$PWD/.release/clumpy
    clumpy help

Generate two octaves of simplex noise and combine them:

    clumpy generate_simplex 500x250 0.5 16.0 0 noise1.npy
    clumpy generate_simplex 500x250 1.0 8.0  0 noise2.npy

    python3 <<EOL
    import numpy as np; from PIL import Image
    noise1, noise2 = np.load("noise1.npy"), np.load("noise2.npy")
    result = np.clip(np.abs(noise1 + noise2), 0, 1)
    Image.fromarray(np.uint8(result * 255), "L").show()
    EOL

<img src="https://github.com/prideout/clumpy/raw/master/extras/example1.png">

Create a distance field with a random shape:

    clumpy generate_dshapes 500x250 1 0 shapes.npy
    clumpy visualize_sdf shapes.npy shapes.npy

    python3 <<EOL
    import numpy as np; from PIL import Image
    Image.fromarray(np.load('shapes.npy'), 'RGB').show()
    EOL

<img src="https://github.com/prideout/clumpy/raw/master/extras/example2.png">

Create a 2x2 atlas of distance fields, each with 5 random shapes:

    for i in {1..4}; do clumpy generate_dshapes 250x125 5 $i shapes$i.npy; done
    for i in {1..4}; do clumpy visualize_sdf shapes$i.npy shapes$i.npy; done
    
    python3 <<EOL
    import numpy as np; from PIL import Image
    a, b, c, d = (np.load('shapes{}.npy'.format(i)) for i in [1,2,3,4])
    img = np.vstack(((np.hstack((a,b)), np.hstack((c,d)))))
    Image.fromarray(img, 'RGB').show()
    EOL

<img src="https://github.com/prideout/clumpy/raw/master/extras/example3.png">

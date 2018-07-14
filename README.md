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
    clumpy visualize_sdf shapes.npy shapeviz.npy

    python3 <<EOL
    import numpy as np; from PIL import Image
    Image.fromarray(np.load('shapeviz.npy'), 'RGB').show()
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

<!-- TODO items

travis

Look at type_code in cnpy

Create a nice point distribution, cull points that overlap certain areas, then plot them:
Look at type_code in cnpy

test_clumpy should have an exec lambda: checks return codes and splits strings.

beeline to simple movie (no streamlines, no wide points, but YES to a seamless loop)
    clumpy cull_points <input_pts> <sdf_file> <output_pts>
    clumpy advect_points <input_pts> <velocities_img> <time_step> <nframes> <output_img_suffix>

Import a bitmap, generate a distance field from it, add noise, and export:

    python3 <<EOL
    from PIL import Image;
    Image.load('foo.png').toarray().save('foo.npy')
    EOL

    flesh out splat_points

    clumpy generate_svg <input_file> <output_file>

    angles_to_vectors <input_file> <output_file>
        https://docs.scipy.org/doc/numpy/reference/routines.math.html

    variable_blur
        https://github.com/scipy/scipy/blob/master/scipy/ndimage/filters.py#L213

    gradient_magnitude (similar to curl2d)
        https://docs.scipy.org/doc/numpy/reference/routines.math.html

    repro heman stuff
        note that even a color gradient could be achieved; search for  "color lookup" here:
            https://docs.scipy.org/doc/numpy-1.12.0/user/basics.indexing.html
        look at pillow example here (although it should have h=1, then resize)
            https://stackoverflow.com/questions/25668828/how-to-create-colour-gradient-in-python

    https://github.com/prideout/reba-island
    https://blind.guru/simple_cxx11_workqueue.html
    https://matplotlib.org/gallery/images_contours_and_fields/quiver_demo.html#sphx-glr-gallery-images-contours-and-fields-quiver-demo-py

    
-->

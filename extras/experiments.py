#!/usr/bin/env python3

import numpy as np
from PIL import Image

# Create 8x4 image
img = np.zeros([4, 8])
img.fill(0.25)

# Plot a list of points using "column,row".
pts = np.transpose(np.array([[7, 3], [2, 1]], dtype=np.intp))
img[pts[1], pts[0]] = 0.7

# Add a value to a set of pixels.
img[pts[1], pts[0]] = img[pts[1], pts[0]] + 0.3

print(img)

if False:
    img = Image.fromarray(np.uint8(img * 255), 'L')
    img = img.resize([1000, 500], resample=Image.NEAREST)
    img.show()

# Get the nth row of Pascal's triangle.
def get_weights(n):
    rows = [[1],[1,1]]
    for j in range(2, n + 1):
        prev = rows[j - 1]
        newrow = [1]
        for k in range(1, j - 1):
            newrow.append(prev[k] + prev[k - 1])
        newrow.append(1)
        rows.append(newrow)
    return np.array(rows[-1])

# Generate a nxn Gaussian kernel.
def get_gaussian(n):
    v = get_weights(n)
    k = np.transpose(np.mat(v)) * v
    return k / np.sum(k)

k = get_gaussian(5)
print(k)

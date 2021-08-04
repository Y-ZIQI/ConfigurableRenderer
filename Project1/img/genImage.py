import sys, os
import numpy as np
from PIL import Image

path = "newimage"
outpath = "newimage.png"

if __name__ == "__main__":
    if len(sys.argv) > 1:
        path = sys.argv[1]
    if len(sys.argv) > 2:
        outpath = sys.argv[1]
    try:
        with open(path, "rb") as f:
            arr = f.read()
    except:
        print("File read failed.")
        sys.exit(2)
    alist = list(arr)
    w, h = alist[0] * 100 + alist[1], alist[2] * 100 + alist[3]
    arr = np.array(alist[4:], np.uint8)
    arr = arr.reshape([h, w, 3])

    im = Image.fromarray(arr)
    flip_image= im.transpose(Image.FLIP_TOP_BOTTOM)
    flip_image.save(outpath)

    if os.path.exists(path):
        os.remove(path)


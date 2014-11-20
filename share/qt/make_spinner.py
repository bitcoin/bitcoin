#!/usr/bin/env python
# W.J. van der Laan, 2011
# Make spinning animation from a .png
# Requires imagemagick 6.7+
from __future__ import division
from os import path
from PIL import Image
from subprocess import Popen

SRC='img/reload.png'
TMPDIR='../../src/qt/res/movies/'
TMPNAME='spinner-%03i.png'
NUMFRAMES=35
FRAMERATE=10.0
CONVERT='convert'
CLOCKWISE=True
DSIZE=(16,16)

im_src = Image.open(SRC)

if CLOCKWISE:
    im_src = im_src.transpose(Image.FLIP_LEFT_RIGHT)

def frame_to_filename(frame):
    return path.join(TMPDIR, TMPNAME % frame)

frame_files = []
for frame in xrange(NUMFRAMES):
    rotation = (frame + 0.5) / NUMFRAMES * 360.0
    if CLOCKWISE:
        rotation = -rotation
    im_new = im_src.rotate(rotation, Image.BICUBIC)
    im_new.thumbnail(DSIZE, Image.ANTIALIAS)
    outfile = frame_to_filename(frame)
    im_new.save(outfile, 'png')
    frame_files.append(outfile)



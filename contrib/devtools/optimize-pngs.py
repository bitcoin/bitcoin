#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Run this script every time you change one of the png files. Using pngcrush, it will optimize the png files, remove various color profiles, remove ancillary chunks (alla) and text chunks (text).
#pngcrush -brute -ow -rem gAMA -rem cHRM -rem iCCP -rem sRGB -rem alla -rem text
'''
import os
import sys
import subprocess
import hashlib
from PIL import Image  # pip3 install Pillow

def file_hash(filename):
    '''Return hash of raw file contents'''
    with open(filename, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()

def content_hash(filename):
    '''Return hash of RGBA contents of image'''
    i = Image.open(filename)
    i = i.convert('RGBA')
    data = i.tobytes()
    return hashlib.sha256(data).hexdigest()

pngcrush = 'pngcrush'
git = 'git'
folders = [
    "src/qt/res/animation",
    "src/qt/res/icons",
    "src/qt/res/images",
    "share/pixmaps"
]
basePath = subprocess.check_output([git, 'rev-parse', '--show-toplevel'], universal_newlines=True, encoding='utf8').rstrip('\n')
totalSaveBytes = 0
noHashChange = True

outputArray = []
for folder in folders:
    absFolder=os.path.join(basePath, folder)
    for file in os.listdir(absFolder):
        extension = os.path.splitext(file)[1]
        if extension.lower() == '.png':
            print("optimizing {}...".format(file), end =' ')
            file_path = os.path.join(absFolder, file)
            fileMetaMap = {'file' : file, 'osize': os.path.getsize(file_path), 'sha256Old' : file_hash(file_path)}
            fileMetaMap['contentHashPre'] = content_hash(file_path)
            try:
                subprocess.call([pngcrush, "-brute", "-ow", "-rem", "gAMA", "-rem", "cHRM", "-rem", "iCCP", "-rem", "sRGB", "-rem", "alla", "-rem", "text", file_path],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except:
                print("pngcrush is not installed, aborting...")
                sys.exit(0)

            #verify
            if "Not a PNG file" in subprocess.check_output([pngcrush, "-n", "-v", file_path], stderr=subprocess.STDOUT, universal_newlines=True, encoding='utf8'):
                print("PNG file "+file+" is corrupted after crushing, check out pngcursh version")
                sys.exit(1)

            fileMetaMap['sha256New'] = file_hash(file_path)
            fileMetaMap['contentHashPost'] = content_hash(file_path)

            if fileMetaMap['contentHashPre'] != fileMetaMap['contentHashPost']:
                print("Image contents of PNG file {} before and after crushing don't match".format(file))
                sys.exit(1)

            fileMetaMap['psize'] = os.path.getsize(file_path)
            outputArray.append(fileMetaMap)
            print("done")

print("summary:\n+++++++++++++++++")
for fileDict in outputArray:
    oldHash = fileDict['sha256Old']
    newHash = fileDict['sha256New']
    totalSaveBytes += fileDict['osize'] - fileDict['psize']
    noHashChange = noHashChange and (oldHash == newHash)
    print(fileDict['file']+"\n  size diff from: "+str(fileDict['osize'])+" to: "+str(fileDict['psize'])+"\n  old sha256: "+oldHash+"\n  new sha256: "+newHash+"\n")

print("completed. Checksum stable: "+str(noHashChange)+". Total reduction: "+str(totalSaveBytes)+" bytes")

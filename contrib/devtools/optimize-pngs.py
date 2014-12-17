#!/usr/bin/env python

import os
import sys
import subprocess

#optimize png, remove various color profiles, remove ancillary chunks (alla) and text chunks (text)
#pngcrush -brute -ow -rem gAMA -rem cHRM -rem iCCP -rem sRGB -rem alla -rem text

folders = ["src/qt/res/movies", "src/qt/res/icons", "src/qt/res/images"]
basePath = subprocess.check_output("git rev-parse --show-toplevel", shell=True).rstrip('\n')
totalSaveBytes = 0

outputArray = []
for folder in folders:
    absFolder=os.path.join(basePath, folder)
    for file in os.listdir(absFolder):
        extension = os.path.splitext(file)[1]
        if extension.lower() == '.png':
            print("optimizing "+file+"..."),
            file_path = os.path.join(absFolder, file)
            fileMetaMap = {'file' : file, 'osize': os.path.getsize(file_path), 'sha256Old' : subprocess.check_output("openssl dgst -sha256 "+file_path, shell=True).rstrip('\n')};
        
            pngCrushOutput = ""
            try:
                pngCrushOutput = subprocess.check_output("pngcrush -brute -ow -rem gAMA -rem cHRM -rem iCCP -rem sRGB -rem alla -rem text "+file_path+" >/dev/null 2>&1", shell=True).rstrip('\n')
            except:
                print "pngcrush is not installed, aborting..."
                sys.exit(0)
        
            #verify
            if "Not a PNG file" in subprocess.check_output("pngcrush -n -v "+file_path+" >/dev/null 2>&1", shell=True):
                print "PNG file "+file+" is corrupted after crushing, check out pngcursh version"
                sys.exit(1)
            
        
            fileMetaMap['sha256New'] = subprocess.check_output("openssl dgst -sha256 "+file_path, shell=True).rstrip('\n')
            fileMetaMap['psize'] = os.path.getsize(file_path)
            outputArray.append(fileMetaMap)
            print("done\n"),

print "summary:\n+++++++++++++++++"
for fileDict in outputArray:
    oldHash = fileDict['sha256Old'].split("= ")[1]
    newHash = fileDict['sha256New'].split("= ")[1]
    totalSaveBytes += fileDict['osize'] - fileDict['psize']
    print fileDict['file']+"\n  size diff from: "+str(fileDict['osize'])+" to: "+str(fileDict['psize'])+"\n  old sha256: "+oldHash+"\n  new sha256: "+newHash+"\n"
    
print "completed. Total reduction: "+str(totalSaveBytes)+" bytes"
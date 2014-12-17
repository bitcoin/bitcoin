#!/usr/bin/env python
'''
Run this script inside of src/ and it will look for all the files
that were changed this year that still have the last year in the
copyright headers, and it will fix the headers on that file using
a perl regex one liner.

For example: if it finds something like this and we're in 2014

// Copyright (c) 2009-2013 The Bitcoin Core developers

it will change it to

// Copyright (c) 2009-2014 The Bitcoin Core developers

It will do this for all the files in the folder and its children.

Author: @gubatron
'''
import os
import time

year = time.gmtime()[0]
last_year = year - 1
command = "perl -pi -e 's/%s The Bitcoin/%s The Bitcoin/' %s"
listFilesCommand = "find . | grep %s"

extensions = [".cpp",".h"]

def getLastGitModifiedDate(filePath):
  gitGetLastCommitDateCommand = "git log " + filePath +" | grep Date | head -n 1"
  p = os.popen(gitGetLastCommitDateCommand)
  result = ""
  for l in p:
    result = l
    break
  result = result.replace("\n","")
  return result

n=1
for extension in extensions:
  foundFiles = os.popen(listFilesCommand % extension)
  for filePath in foundFiles:
    filePath = filePath[1:-1]
    if filePath.endswith(extension):
      filePath = os.getcwd() + filePath
      modifiedTime = getLastGitModifiedDate(filePath)
      if len(modifiedTime) > 0 and str(year) in modifiedTime:
        print n,"Last Git Modified: ", modifiedTime, " - ", filePath
        os.popen(command % (last_year,year,filePath))
        n = n + 1



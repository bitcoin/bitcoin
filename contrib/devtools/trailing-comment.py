#!/usr/bin/env python
"""
Searches for all instances of trailing single line comments where the total line
length exceeds the provided line length, and converts to a separate comment line
then code line.

For example:
void my_func(int param); // This is a very long comment explaining what this function does.

becomes:
// This is a very long comment explaining what this function does.
void my_func(int param);

Copyright (c) 2017 The Bitcoin Unlimited developers
Distributed under the MIT software license, see the accompanying
file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
import pdb
import sys
import types

TAB_SIZE = 8

def main(files, cutoff,verbose=False):
    if not files:
        files.append(sys.stdin)
    for f in files:
        changed = False
        if type(f) in types.StringTypes:
            fo = open(f,"r")
            if verbose:
                print("Processing %s" % f)
        else:
            fo = f
            verbose = False # if using stdin/stdout, don't clutter it with logging
            
        lines = fo.readlines()      
        output = []
        for line in lines:
            #print len(line), "//" in line, line
            line = line.replace("\t",TAB_SIZE*" ")
            if len(line) > cutoff and "//" in line:
                try:
                    (code, comment) = line.rsplit("//",1)
                except ValueError:
                    print line
                    pdb.set_trace()
                # If there is a quote both the code and the comment, the // is likely in a comment    
                if comment.count('"')%2==1 and code.count('"')%2==1:
                    print("Warning, this line is weird, not touching...")
                    print(line)
                    output.append(line)
                    continue
                codeLen = len(code.lstrip())
                indentation = len(code) - codeLen
                if codeLen: # trailing comments has at least some code on the line
                    # pdb.set_trace()
                    newComment = " "*indentation + "//" + comment
                    if verbose:
                        print("Was:\n%s" % line.rstrip())
                        print("Now:")
                        print(newComment.rstrip())
                        print(code.rstrip())
                        #sys.stdout.write(newComment)
                        #sys.stdout.write(code)
                    output.append(newComment)
                    output.append(code.rstrip() + "\n")
                    changed = True
                else:
                    output.append(line)
            else:
                output.append(line)
        if type(f) in types.StringTypes:
            if changed:
                print("%s changed" % f)        
                fo = open(f,"w")
                fo.writelines(output)
        else:
            sys.stdout.writelines(output)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("usage: trailing-comment.py <max columns> [files]")
        sys.exit(-1)
    main(sys.argv[2:],int(sys.argv[1]))

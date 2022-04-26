import os
import platform
import subprocess

is_msys = False
if os.sep == '/' and platform.system() == 'Windows':
    is_msys = True

def posix_to_windows_path(filename):

    if is_msys:
        result = subprocess.run(['cygpath', '-w', filename], stdout=subprocess.PIPE)
        filename = result.stdout.rstrip()
        filename = str(filename,'utf-8')

    return filename

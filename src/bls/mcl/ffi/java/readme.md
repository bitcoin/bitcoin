# MCL Java Bindings

## How to build with CMake
Build mcl with CMake first (see top-level readme).
The following commands assume building with Release configuration, exchange Release with Debug otherwise.

For Linux, macOS, etc.
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
For Visual Studio,
```bash
mkdir build
cd build
cmake .. -A x64 -DMCL_LINK_DIR=C:\[Your MCL Directory location]\mcl\build\lib\Release\ -DGMP_LINK_DIR=C:\[Your MCL Directory location]\mcl\external\cybozulib_ext\lib\mt\14\ ..
msbuild mcl.sln /p:Configuration=Release /m
```
On Windows, make sure to either specify `-DMCL_DOWNLOAD_SOURCE=ON` when building mcl or provide the location of mpir.lib or gmp.lib to GMP_LINK_DIR.
If GMP is not found, the build will try to link the mcljava.dll without linking against GMP. Thus, if you specified -DMCL_USE_GMP=OFF when building MCL, do not specify -DGMP_LINK_DIR here and the build should work.
On Windows, you might need to delete everything from link_mpir.hpp and re-build both mcl and mcljava, as the lib-pragma in this header might be incompatible with the GMP directory set above.
On Linux, either install libgmp.so and libmcl.so to /usr/lib or use the MCL_LINK_DIR and GMP_LINK_DIR options to specify alternative directories.
On both Linux and Windows, only pass absolute paths to the above options.

Finally, if you specified `-DMCL_USE_XBYAK=OFF` to the compilation of mcl, be sure to specify it here again.


## How to test
Within the build dir, do
```bash
ctest -C Release --verbose
```

## How to use

On Linux and Mac, you can install libmcljava.so and libmclelgamal.so in the /usr/lib directory in order to be able to use them for all Java Projects.
On Windows, you can copy libmcljava.dll and libmclelgamal.dll into the working directory of the Java process to be executed.
Alternatively, on both Linux/Mac and Windows, you can use the `-Djava.library.path=` option and install the .so or .dll libraries in any directory. You should only use absolute paths for this.

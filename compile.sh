CXX='g++' make -j8 &
CXX='g++ -m32' make -j8 &
AR='x86_64-w64-mingw32-ar' RANLIB='x86_64-w64-mingw32-ranlib' CXX='x86_64-w64-mingw32-g++' CC='x86_64-w64-mingw32-gcc' make -j8
AR='i686-w64-mingw32-ar' RANLIB='i686-w64-mingw32-ranlib' CXX='i686-w64-mingw32-g++' CC='i686-w64-mingw32-gcc' make -j8
wait

CXX='g++' make install &
CXX='g++ -m32' make install &
AR='x86_64-w64-mingw32-ar' RANLIB='x86_64-w64-mingw32-ranlib' CXX='x86_64-w64-mingw32-g++' CC='x86_64-w64-mingw32-gcc' make install &
AR='i686-w64-mingw32-ar' RANLIB='i686-w64-mingw32-ranlib' CXX='i686-w64-mingw32-g++' CC='i686-w64-mingw32-gcc' make install
wait

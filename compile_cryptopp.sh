make clean
CXX='g++' make -j
sudo cp libcryptopp.a /usr/local/lib/libcryptopp.a
sudo make install
make clean
CXX='g++ -m32' make -j
sudo cp libcryptopp.a /usr/local/lib/libcryptopp32.a
make clean
AR='x86_64-w64-mingw32-ar' RANLIB='x86_64-w64-mingw32-ranlib' CXX='x86_64-w64-mingw32-g++' CC='x86_64-w64-mingw32-gcc' make -j
sudo cp libcryptopp.a /usr/local/lib/libcryptoppwin.a
make clean
AR='i686-w64-mingw32-ar' RANLIB='i686-w64-mingw32-ranlib' CXX='i686-w64-mingw32-g++' CC='i686-w64-mingw32-gcc' make -j
sudo cp libcryptopp.a /usr/local/lib/libcryptopp32win.a

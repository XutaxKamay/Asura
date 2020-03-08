make clean
CXX='g++' make -j8
sudo cp libcryptopp.a /usr/local/lib/libcryptopp.a
sudo make install
make clean
CXX='g++ -m32' make -j8
sudo cp libcryptopp.a /usr/local/lib/libcryptopp32.a
make clean
CXX='x86_64-w64-mingw32-g++' make -j8
sudo cp libcryptopp.a /usr/local/lib/libcryptoppwin.a
make clean
CXX='i686-w64-mingw32-g++' make -j8
sudo cp libcryptopp.a /usr/local/lib/libcryptopp32win.a

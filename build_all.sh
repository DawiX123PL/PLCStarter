#!/bin/sh


sudo apt install -y gcc make build-essential git cmake dos2unix openssh-server

cd boost_1_79_0 

dos2unix ./bootstrap.sh 
dos2unix ./tools/build/src/engine/build.sh

chmod +x ./bootstrap.sh
chmod +x ./tools/build/src/engine/build.sh


./bootstrap.sh

# compile on one core - to prevent memory overflow
# and reduce memory usage by tunning garbage collector settings
./b2 -j 1 compileflags="--param ggc-min-expand=1" compileflags="--param ggc-min-heapsize=700" linkflags="--param ggc-min-expand=1" linkflags="--param ggc-min-heapsize=700"

#./b2 -j 12


cd ..

cmake .
make



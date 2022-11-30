#!/bin/sh


chmod +x ./boost_1_79_0/bootstrap.sh
chmod +x ./boost_1_79_0/tools/build/src/engine/build.sh


# compile on one core - to prevent memory overflow
# and reduce memory usage by tunning garbage collector settings
cd boost_1_79_0 
./b2 -j 1 compileflags="--param ggc-min-expand=1" compileflags="--param ggc-min-heapsize=700" linkflags="--param ggc-min-expand=1" linkflags="--param ggc-min-heapsize=700"



#!/bin/bash

this_dir=`pwd`

for i in *.doxy
do
    doxygen $i
    pushd ../build/latex
    make
    o=${i%.*}
    cp refman.pdf $this_dir/$o.pdf
    popd
done

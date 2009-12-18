#!/bin/bash

this_dir=`pwd`

for i in *.doxy
do
#    (cat $i; bzr version-info --custom --template="PROJECT_NUMBER = \"bzr revision: {revno} | bzr revision id: {revision_id}\"") | doxygen -

    doxygen $i
    
    pushd ../build/latex
    make
    o=${i%.*}
    cp refman.pdf $this_dir/$o.pdf
    popd
done

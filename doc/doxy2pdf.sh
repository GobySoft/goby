#!/bin/bash

this_dir=`pwd`

for i in *.doxy
do
   (cat $i; bzr version-info --custom --template="PROJECT_NUMBER = \"bzr revision: {revno} | bzr revision id: {revision_id}\"") | doxygen -

   #doxygen $i
    
    pushd ../build/latex
    cat refman.tex | sed 's/\\chapter{Namespace Index}/\\appendix\\chapter{Namespace Index}/'  | sed 's/\\include/\\input/' > refman2.tex
    mv refman2.tex refman.tex
    make
    o=${i%.*}
    cp refman.pdf $this_dir/$o.pdf
    popd
done

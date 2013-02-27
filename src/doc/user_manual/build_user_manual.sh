#!/bin/bash

# you need xetex and gentium-plus to build this manual
# > sudo apt-get install texlive-xetex fonts-sil-gentium

set -e

CMAKE_CURRENT_SOURCE_DIR=$2
CMAKE_CURRENT_BINARY_DIR=$3
OUTPUT=$4

if [[ "$1" == "quick" ]]; then
    xelatex -shell-escape -halt-on-error ${CMAKE_CURRENT_BINARY_DIR}/user_manual.tex
    cp user_manual.pdf $OUTPUT
else    
    rm -f ${CMAKE_CURRENT_BINARY_DIR}/user_manual.aux
    pushd ${CMAKE_CURRENT_BINARY_DIR}
    xelatex -shell-escape user_manual.tex -halt-on-error -no-pdf
    bibtex user_manual
    makeglossaries user_manual
    xelatex -shell-escape user_manual.tex -halt-on-error -no-pdf
    xelatex -shell-escape user_manual.tex
    cp user_manual.pdf $OUTPUT
    popd
fi


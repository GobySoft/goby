#!/bin/bash

# you need xetex and gentium-plus to build this manual
# > sudo apt-get install texlive-xetex
# > wget "http://scripts.sil.org/cms/scripts/render_download.php?&format=file&media_id=GentiumPlus-1.504-developer.deb&filename=fonts-sil-gentium-plus_1.504-developer-1_all.deb" -O gentium.deb
# > sudo dpkg -i gentium.deb

set -e

CMAKE_CURRENT_SOURCE_DIR=$2
CMAKE_CURRENT_BINARY_DIR=$3
OUTPUT=$4

if [[ "$1" == "quick" ]]; then
    xelatex -halt-on-error ${CMAKE_CURRENT_BINARY_DIR}/user_manual.tex
    cp user_manual.pdf $OUTPUT
else    
    rm -f ${CMAKE_CURRENT_BINARY_DIR}/user_manual.aux
    pushd ${CMAKE_CURRENT_BINARY_DIR}
    xelatex user_manual.tex -halt-on-error -no-pdf
    bibtex user_manual
    makeglossaries user_manual
    xelatex user_manual.tex -halt-on-error -no-pdf
    xelatex user_manual.tex
    cp user_manual.pdf $OUTPUT
    popd
fi


#!/bin/bash

# you need xetex and gentium-plus to build this manual
# > sudo apt-get install texlive-xetex
# > wget "http://scripts.sil.org/cms/scripts/render_download.php?&format=file&media_id=GentiumPlus-1.504-developer.deb&filename=fonts-sil-gentium-plus_1.504-developer-1_all.deb" -O gentium.deb
# > sudo dpkg -i gentium.deb

set -e

if [[ "$1" == "quick" ]]; then
    xelatex -halt-on-error user_manual.tex
else
    rm -f user_manual.aux
    xelatex user_manual.tex -halt-on-error -no-pdf
    bibtex user_manual
    makeglossaries user_manual
    xelatex user_manual.tex -halt-on-error -no-pdf
    xelatex user_manual.tex
    gnome-open user_manual.pdf
fi